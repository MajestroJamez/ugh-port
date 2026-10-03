#include "ReplayPlayer.hpp"

#include <cctype>
#include <cstdio>
#include <utility>

namespace ugh::tool {

bool ReplayPlayer::play(const std::string& path, Mode mode) {
    ReplayFile file(path);
    if (!file.open()) {
        std::fprintf(stderr, "%s\n", file.error().c_str());
        return false;
    }
    cores_.reset();
    // a tick is played when the next one is read: the last one of the game is known then
    Tick previous, current;
    bool started = false;
    while (file.next(current)) {
        if (started) {
            if (mode == Mode::EachTransition) playTransition(previous, current);
            else playWhole(previous, false);
        }
        previous = std::move(current);
        started = true;
    }
    if (!file.error().empty()) {
        std::fprintf(stderr, "%s\n", file.error().c_str());
        return false;
    }
    if (started && mode == Mode::WholeGame) playWhole(previous, true);
    return true;
}

/** Tick 0 sets the start, every later tick is one step of the core. */
void ReplayPlayer::playWhole(const Tick& tick, bool last) {
    if (tick.number == 0) {
        for (const auto& [name, value] : tick.state) set(name, value);
    } else {
        int end = cores_.step();
        if (end < 0) problem(tick.number, "frame", "the cores disagree whether the game ended");
        else if (end != UGH_SIM_CONTINUE && !last)
            problem(tick.number, "frame", "the core ended the game (" + std::to_string(end) + ")");
        compare("frame", tick, tick.state, groups(), Unknown::TakeOver, true);
    }
    for (const auto& [name, value] : tick.inject) set(name, value);
    for (int key : tick.keys) cores_.key(key);
    takeProblems();
}

void ReplayPlayer::playTransition(const Tick& before, const Tick& after) {
    const std::string& from = phase(before);
    const std::string& to = phase(after);
    if (from == "start" && to == "betweenLevels") {
        load(before);
        cores_.newGame();
        compare("new game", after, after.state, {"game."}, Unknown::Count, false);
    } else if (to == "caption" && (from == "betweenLevels" || from == "play")) {
        checkLevelStart(before, after);
    } else if (from == "play" && to == "play") {
        load(before);
        cores_.playFrame();
        compare("play frame", after, undoMissingStages(after), groups(), Unknown::Error, false);
    } else {
        report_.skipped();
    }
    takeProblems();
}

void ReplayPlayer::checkLevelStart(const Tick& before, const Tick& after) {
    load(before);
    if (phase(before) == "play") {
        // the last frame of the level ran before the loop saw the end of the fade-out
        cores_.playFrame();
        if (cores_.levelEnd() != UGH_SIM_CONTINUE) {
            problem(after.number, "level start", "the core ended the game");
            return;
        }
    }
    cores_.levelStart();
    // what missing stages changed in that last frame is partly overwritten by the load: not compared here
    Fields expected = after.state;
    for (const auto& [stage, fields] : after.before)
        if (!ugh_sim_has_stage(stage.c_str()))
            for (const auto& field : fields) expected.erase(field.first);
    compare("level start", after, expected, groups(), Unknown::Count, false);
}

/** The recorded state before a transition, with its injections and keys. */
void ReplayPlayer::load(const Tick& tick) {
    cores_.clear();
    for (const auto& [name, value] : tick.state) set(name, value);
    for (const auto& [name, value] : tick.inject) set(name, value);
    for (int key : tick.keys) cores_.key(key);
}

void ReplayPlayer::set(const std::string& name, const std::string& value) {
    if (cores_.set(name, value) < 0) problem(NO_TICK, "set", "bad value " + name + "=" + value);
}

void ReplayPlayer::problem(long long tick, const std::string& what, const std::string& text) {
    report_.problem(tick, what, text);
}

void ReplayPlayer::takeProblems() {
    cores_.takeProblems(
        [](void* ctx, const char* text) { static_cast<ReplayPlayer*>(ctx)->problem(NO_TICK, "core", text); }, this);
}

void ReplayPlayer::compare(const std::string& what, const Tick& tick, const Fields& expected,
                           const std::vector<const char*>& groups, Unknown unknown, bool takeOverMismatches) {
    report_.checked(what);
    Fields actual = cores_.known();
    std::string diffs;
    int n = 0;
    for (const auto& [name, value] : expected) {
        if (name == "game.phase") continue;
        bool inGroup = false;
        for (const char* group : groups) inGroup = inGroup || startsWith(name, group);
        if (!inGroup) continue;
        auto it = actual.find(name);
        if (it == actual.end()) {
            switch (unknown) {
                case Unknown::Count: report_.unknown(name); break;
                case Unknown::Error: diffs += " " + name + " unknown (expected " + value + ")"; n++; break;
                case Unknown::TakeOver: set(name, value); report_.takenOver(general(name), tick.number); break;
            }
            continue;
        }
        report_.compared(name.substr(0, name.find('.')));
        if (it->second != value) {
            diffs += " " + name + " expected " + value + " got " + it->second;
            n++;
            if (takeOverMismatches) set(name, value);
        }
    }
    if (n > 0) problem(tick.number, what, std::to_string(n) + " fields:" + diffs);
}

/** The field groups of the stages the core has (passengers: their pads too). */
std::vector<const char*> ReplayPlayer::groups() {
    std::vector<const char*> g = {"game.", "copter."};
    if (ugh_sim_has_stage("passengers")) {
        g.push_back("pad.");
        g.push_back("passenger.");
    }
    if (ugh_sim_has_stage("objects")) g.push_back("object.");
    if (ugh_sim_has_stage("bonuses")) g.push_back("bonus.");
    return g;
}

/** The recorded state after the tick without what the stages the core lacks changed (B lines, last first). */
Fields ReplayPlayer::undoMissingStages(const Tick& tick) {
    Fields expected = tick.state;
    for (auto it = tick.before.rbegin(); it != tick.before.rend(); ++it) {
        if (ugh_sim_has_stage(it->first.c_str())) continue;
        for (const auto& [name, value] : it->second) {
            if (value == "~") expected.erase(name);
            else expected[name] = value;
        }
    }
    return expected;
}

/** passenger.3.x -> passenger.N.x */
std::string ReplayPlayer::general(const std::string& name) {
    std::string g;
    for (size_t i = 0; i < name.size(); i++) {
        if (std::isdigit(static_cast<unsigned char>(name[i])) && i > 0 && name[i - 1] == '.') {
            while (i + 1 < name.size() && std::isdigit(static_cast<unsigned char>(name[i + 1]))) i++;
            g += 'N';
        } else {
            g += name[i];
        }
    }
    return g;
}

const std::string& ReplayPlayer::phase(const Tick& tick) {
    static const std::string none;
    auto it = tick.state.find("game.phase");
    return it == tick.state.end() ? none : it->second;
}

}  // namespace ugh::tool
