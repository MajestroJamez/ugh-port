// Replay player: checks the C++ core against golden replays (format "UGR 0", see
// verify/src/test/kotlin/ugh/verify/replay/ReplayWriter.kt).
//
//   ugh_replay [--each] <ugh-sim.bin> <replay.ugr> ...
//
// Default: the whole game. The core starts from the state of tick 0 (the start of a new game) and runs on its
// own (ugh_sim_step), getting only the keys and the injections (I lines) of the recording; after every tick all
// fields are compared with the recording. A field the core has never written (memory left by the screens
// before the game, e.g. passengers of the attract mode) is taken over the first time the recording shows it;
// a mismatch is reported and the recorded value taken over, so one error does not hide the next ones.
//
// --each: every transition on its own, from the recorded state before the tick (T line, I line, keys);
// stages the core does not have are undone on the expected state (B lines):
//   new game     start -> betweenLevels             game.*
//   level start  betweenLevels / play -> caption    game.*, copter.* and the groups of the stages the core has
//                                                  (pad.* with passenger.*, object.*, bonus.*; from play: its last
//                                                  frame and the level end first)
//   play frame   play -> play                       the same (every field must be known)
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ugh_sim.h"

namespace {

using Fields = std::map<std::string, std::string>;

struct Tick {
    long long number = -1;
    Fields state;                          // full state after the tick
    std::vector<int> keys;                 // delivered after it
    std::vector<std::pair<std::string, Fields>> before;  // B lines, in stage order
    Fields inject;                         // I line
};

Fields pairs(const std::string& text) {
    Fields f;
    std::istringstream in(text);
    std::string item;
    while (in >> item) {
        auto eq = item.find('=');
        if (eq != std::string::npos) f[item.substr(0, eq)] = item.substr(eq + 1);
    }
    return f;
}

bool startsWith(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

struct Result {
    std::map<std::string, long long> checked;   // transition -> ticks
    long long skipped = 0;
    long long mismatches = 0;
    long long adopted = 0;                      // values the core took over (never written by it)
    std::map<std::string, long long> adoptedFields;  // field (index as N) -> first tick
    std::vector<std::string> reports;
    std::map<std::string, long long> unknown;   // field -> ticks where the core did not know it (--each, level start)
    std::map<std::string, long long> compared;  // field group -> values compared
};

/** What to do with a recorded field the core does not know. */
enum class Unknown { COUNT, ERROR, ADOPT };

class Player {
public:
    Player(ugh_sim* sim, Result& result) : sim_(sim), r_(result) {}

    /** The whole game: tick 0 sets the start, every later tick is one step of the core. */
    void whole(const Tick& t, bool last) {
        if (t.number == 0) {
            for (const auto& [name, value] : t.state) set(name, value);
        } else {
            int end = ugh_sim_step(sim_);
            if (end != UGH_SIM_CONTINUE && !last) report(t, "frame", "the core ended the game (" + std::to_string(end) + ")");
            compare("frame", t, t.state, groups(), Unknown::ADOPT, true);
        }
        for (const auto& [name, value] : t.inject) set(name, value);
        for (int k : t.keys) ugh_sim_key(sim_, k);
        takeProblems();
    }

    /** One transition from the recorded state before it. */
    void each(const Tick& prev, const Tick& cur) {
        const std::string& from = phase(prev);
        const std::string& to = phase(cur);
        if (from == "start" && to == "betweenLevels") {
            load(prev);
            ugh_sim_new_game(sim_);
            compare("new game", cur, cur.state, {"game."}, Unknown::COUNT, false);
        } else if (to == "caption" && (from == "betweenLevels" || from == "play")) {
            load(prev);
            if (from == "play") {
                // the last frame of the level ran before the loop saw the end of the fade-out
                ugh_sim_play_frame(sim_);
                if (ugh_sim_level_end(sim_) != UGH_SIM_CONTINUE) { report(cur, "level start", "the core ended the game"); return; }
            }
            ugh_sim_level_start(sim_);
            // what missing stages changed in that last frame is partly overwritten by the load: not compared here
            Fields expected = cur.state;
            for (const auto& [stage, fields] : cur.before)
                if (!ugh_sim_has_stage(stage.c_str())) for (const auto& field : fields) expected.erase(field.first);
            compare("level start", cur, expected, groups(), Unknown::COUNT, false);
        } else if (from == "play" && to == "play") {
            load(prev);
            ugh_sim_play_frame(sim_);
            compare("play frame", cur, undoStages(cur), groups(), Unknown::ERROR, false);
        } else {
            r_.skipped++;
        }
        takeProblems();
    }

private:
    ugh_sim* sim_;
    Result& r_;

    /** The field groups of the stages the core has (passengers: their pads too). */
    static std::vector<const char*> groups() {
        std::vector<const char*> g = {"game.", "copter."};
        if (ugh_sim_has_stage("passengers")) { g.push_back("pad."); g.push_back("passenger."); }
        if (ugh_sim_has_stage("objects")) g.push_back("object.");
        if (ugh_sim_has_stage("bonuses")) g.push_back("bonus.");
        return g;
    }

    /** The recorded state after the tick without what the stages the core lacks changed (B lines, last first). */
    static Fields undoStages(const Tick& cur) {
        Fields expected = cur.state;
        for (auto it = cur.before.rbegin(); it != cur.before.rend(); ++it) {
            if (ugh_sim_has_stage(it->first.c_str())) continue;
            for (const auto& [name, value] : it->second) {
                if (value == "~") expected.erase(name); else expected[name] = value;
            }
        }
        return expected;
    }

    /** passenger.3.x -> passenger.N.x */
    static std::string general(const std::string& name) {
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

    static const std::string& phase(const Tick& t) {
        static const std::string none;
        auto it = t.state.find("game.phase");
        return it == t.state.end() ? none : it->second;
    }

    void load(const Tick& prev) {
        ugh_sim_clear(sim_);
        for (const auto& [name, value] : prev.state) set(name, value);
        for (const auto& [name, value] : prev.inject) set(name, value);
        for (int k : prev.keys) ugh_sim_key(sim_, k);
    }

    void set(const std::string& name, const std::string& value) {
        if (ugh_sim_set(sim_, name.c_str(), value.c_str()) < 0) report(Tick{}, "set", "bad value " + name + "=" + value);
    }

    void report(const Tick& t, const std::string& what, const std::string& text) {
        r_.mismatches++;
        if (r_.reports.size() < 10) r_.reports.push_back("tick " + std::to_string(t.number) + " " + what + ": " + text);
    }

    void takeProblems() {
        ugh_sim_take_problems(sim_, [](void* ctx, const char* p) { static_cast<Player*>(ctx)->report(Tick{}, "core", p); }, this);
    }

    void compare(const std::string& what, const Tick& cur, const Fields& expected, const std::vector<const char*>& groups,
                 Unknown unknown, bool takeOver) {
        r_.checked[what]++;
        Fields actual;
        ugh_sim_fields(sim_, [](void* ctx, const char* f, const char* v) { (*static_cast<Fields*>(ctx))[f] = v; }, &actual);
        std::string diffs;
        int n = 0;
        for (const auto& [name, value] : expected) {
            if (name == "game.phase") continue;
            bool inGroup = false;
            for (const char* g : groups) inGroup = inGroup || startsWith(name, g);
            if (!inGroup) continue;
            auto it = actual.find(name);
            if (it == actual.end()) {
                switch (unknown) {
                    case Unknown::COUNT: r_.unknown[name]++; break;
                    case Unknown::ERROR: diffs += " " + name + " unknown (expected " + value + ")"; n++; break;
                    case Unknown::ADOPT: set(name, value); r_.adopted++; r_.adoptedFields.emplace(general(name), cur.number); break;
                }
                continue;
            }
            r_.compared[name.substr(0, name.find('.'))]++;
            if (it->second != value) {
                diffs += " " + name + " expected " + value + " got " + it->second;
                n++;
                if (takeOver) set(name, value);
            }
        }
        if (n > 0) report(cur, what, std::to_string(n) + " fields:" + diffs);
    }
};

bool play(ugh_sim* sim, const std::string& path, bool each, Result& result) {
    std::ifstream in(path);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", path.c_str()); return false; }
    std::string line;
    if (!std::getline(in, line) || line != "UGR 0") { std::fprintf(stderr, "%s: not a UGR 0 file\n", path.c_str()); return false; }
    ugh_sim_reset(sim);
    Player player(sim, result);
    Fields state;
    std::vector<Tick> window;  // ticks still waiting for their B / I lines
    auto flush = [&](bool last) {
        if (each) {
            if (window.size() == 2) { player.each(window[0], window[1]); window.erase(window.begin()); }
        } else if (!window.empty()) {
            player.whole(window.back(), last);
            window.clear();
        }
    };
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || startsWith(line, "meta ")) continue;
        if (startsWith(line, "T ")) {
            flush(false);
            auto bar = line.find(" |");
            std::istringstream head(line.substr(2, bar == std::string::npos ? std::string::npos : bar - 2));
            Tick t;
            std::string item;
            head >> t.number;
            while (head >> item) {
                if (startsWith(item, "k=") && item != "k=-") {
                    std::istringstream codes(item.substr(2));
                    std::string code;
                    while (std::getline(codes, code, ',')) t.keys.push_back(static_cast<int>(std::strtol(code.c_str(), nullptr, 16)));
                }
            }
            if (bar != std::string::npos) {
                for (const auto& [name, value] : pairs(line.substr(bar + 2))) {
                    if (value == "~") state.erase(name); else state[name] = value;
                }
            }
            t.state = state;
            window.push_back(std::move(t));
        } else if (startsWith(line, "B ") && !window.empty()) {
            auto sp = line.find(' ', 2);
            window.back().before.emplace_back(line.substr(2, sp - 2), pairs(sp == std::string::npos ? "" : line.substr(sp + 1)));
        } else if (startsWith(line, "I ") && !window.empty()) {
            window.back().inject = pairs(line.substr(2));
        } else {
            std::fprintf(stderr, "%s: unexpected line: %s\n", path.c_str(), line.c_str());
            return false;
        }
    }
    flush(true);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    int first = 1;
    bool each = false;
    if (argc > 1 && std::strcmp(argv[1], "--each") == 0) { each = true; first = 2; }
    if (argc < first + 2) {
        std::fprintf(stderr, "usage: ugh_replay [--each] <ugh-sim.bin> <replay.ugr> ...\n");
        return 2;
    }
    char err[256];
    ugh_sim* sim = ugh_sim_create(argv[first], err, sizeof err);
    if (!sim) { std::fprintf(stderr, "%s\n", err); return 2; }
    bool ok = true;
    for (int i = first + 1; i < argc; i++) {
        Result r;
        if (!play(sim, argv[i], each, r)) { ok = false; continue; }
        std::string checked;
        for (const auto& [what, n] : r.checked) checked += " " + what + " " + std::to_string(n) + ",";
        std::string compared;
        for (const auto& [group, n] : r.compared) compared += " " + group + " " + std::to_string(n);
        std::printf("%s:%s skipped %lld, %lld mismatches, %lld taken over; values compared:%s\n", argv[i], checked.c_str(),
                    r.skipped, r.mismatches, r.adopted, compared.c_str());
        for (const auto& line : r.reports) std::printf("  %s\n", line.c_str());
        if (!r.unknown.empty()) {
            std::string names;
            for (const auto& [name, n] : r.unknown) names += " " + name;
            std::printf("  not known after a level start:%s\n", names.c_str());
        }
        if (!r.adoptedFields.empty()) {
            std::string names;
            for (const auto& [name, tick] : r.adoptedFields) names += " " + name + "@" + std::to_string(tick);
            std::printf("  taken over (first tick):%s\n", names.c_str());
        }
        if (r.mismatches > 0) ok = false;
    }
    ugh_sim_destroy(sim);
    return ok ? 0 : 1;
}
