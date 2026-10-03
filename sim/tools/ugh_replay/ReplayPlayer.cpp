#include "ReplayPlayer.hpp"

#include <cctype>
#include <map>

#include "AuditTable.hpp"
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
            else if (mode == Mode::Audit) playAudit(previous, path);
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
    if (started && mode == Mode::Audit) playAudit(previous, path);
    return true;
}

namespace {

std::map<std::string, std::pair<long long, std::string>>& violations() {
    static std::map<std::string, std::pair<long long, std::string>> v;
    return v;
}

struct Name {
    std::string group, index, field;   // index empty for game.*
};

Name split(const std::string& name) {
    size_t a = name.find('.');
    std::string group = name.substr(0, a);
    if (group == "game") return {group, "", name.substr(a + 1)};
    size_t b = name.find('.', a + 1);
    return {group, name.substr(a + 1, b - a - 1), name.substr(b + 1)};
}

std::string value(const Fields& f, const std::string& name) {
    auto it = f.find(name);
    return it == f.end() ? "" : it->second;
}

bool isDefined(const Fields& state, const Name& n) {
    if (n.group == "pad") return true;
    std::string prefix = n.group + "." + n.index + ".";
    std::string kind = n.group == "object" ? value(state, prefix + "kind") : "*";
    std::string st = n.group == "game" || n.group == "copter" ? "*" : value(state, prefix + "state");
    std::string phase = value(state, "game.phase");
    if (n.group == "copter" && n.field == "fareMin") {
        std::string target = value(state, prefix + "targetPad");
        return target != "0" && target != "7";
    }
    return audit::definedFields(n.group, kind, st, phase).count(n.field) > 0;
}

std::string describe(const Fields& state, const Name& n) {
    std::string prefix = n.group + "." + n.index + ".";
    std::string d = n.group;
    if (n.group == "object") d += " " + value(state, prefix + "kind");
    if (n.group != "game" && n.group != "copter" && n.group != "pad") d += " " + value(state, prefix + "state");
    return d + " [" + value(state, "game.phase") + "] " + n.field;
}

}  // namespace

void printAuditViolations() {
    for (const auto& [key, v] : violations()) std::printf("VIOLATION %lld\t%s\tfirst %s\n", v.first, key.c_str(), v.second.c_str());
}

void ReplayPlayer::playAudit(const Tick& tick, const std::string& path) {
    if (tick.number == 0) {
        for (const auto& [name, value] : tick.state) set(name, value);
    } else {
        int end = cores_.step();
        if (end < 0) problem(tick.number, "frame", "the cores disagree whether the game ended");
        compareAudit(tick, path);
    }
    poison(tick);
    for (const auto& [name, value] : tick.inject) set(name, value);
    for (int key : tick.keys) cores_.key(key);
    takeProblems();
}

void ReplayPlayer::compareAudit(const Tick& tick, const std::string& path) {
    report_.checked("frame");
    Fields actual = cores_.known();
    for (const auto& [name, expected] : tick.state) {
        if (name == "game.phase") continue;
        Name n = split(name);
        bool d = isDefined(tick.state, n);
        auto it = actual.find(name);
        std::string kind;
        if (it == actual.end()) {
            if (d) kind = "unknown";
        } else {
            report_.compared(n.group);
            if (it->second != expected) kind = d ? "mismatch" : "mismatch-of-undefined";
        }
        if (kind.empty()) continue;
        auto& v = violations()[describe(tick.state, n) + " " + kind];
        if (v.first++ == 0) v.second = path + "@" + std::to_string(tick.number);
        set(name, expected);
    }
}

void ReplayPlayer::poison(const Tick& tick) {
    for (const auto& [name, expected] : tick.state) {
        if (name == "game.phase") continue;
        Name n = split(name);
        if (isDefined(tick.state, n)) continue;
        auto [a, b] = audit::poison(n.group, n.field);
        if (a.empty()) continue;
        if (cores_.setEach(name, a, b) < 0) problem(tick.number, "poison", "bad poison " + name);
    }
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
