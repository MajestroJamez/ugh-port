// Replay player: checks the C++ core against golden replays (format "UGR 0", see
// verify/src/test/kotlin/ugh/verify/replay/ReplayWriter.kt).
//
//   ugh_replay <ugh-sim.bin> <replay.ugr> ...
//
// Tick by tick: the core gets the recorded state before the tick (T line, then the I line injection) and the
// keys delivered after it, runs the transition, and its fields are compared with the recorded state after the
// tick. Stages the core does not have yet are undone on the expected state (B lines). Transitions:
//   new game     start -> betweenLevels             game.*
//   level start  betweenLevels / play -> caption    game.*, copter.*, pad.*, passenger.* (from play: its last
//                                                  frame and the level end first)
//   play frame   play -> play                       the same (every field must be known)
// Other ticks (caption, setup, fades between levels) are not checked yet (plan step 8).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
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
    std::vector<std::string> reports;
    std::map<std::string, long long> unknown;   // field -> ticks where the core did not know it (level start)
    std::map<std::string, long long> compared;  // field group -> values compared
};

class Player {
public:
    Player(ugh_sim* sim, Result& result) : sim_(sim), r_(result) {}

    void step(const Tick& prev, const Tick& cur) {
        const std::string& from = phase(prev);
        const std::string& to = phase(cur);
        if (from == "start" && to == "betweenLevels") {
            load(prev);
            ugh_sim_new_game(sim_);
            compare("new game", cur, cur.state, {"game."}, false);
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
            compare("level start", cur, expected, groups(), false);
        } else if (from == "play" && to == "play") {
            load(prev);
            ugh_sim_play_frame(sim_);
            compare("play frame", cur, undoStages(cur), groups(), true);
        } else {
            r_.skipped++;
        }
        ugh_sim_take_problems(sim_, [](void* ctx, const char* p) { static_cast<Player*>(ctx)->report(Tick{}, "core", p); }, this);
    }

private:
    ugh_sim* sim_;
    Result& r_;

    /** The field groups the core has (passengers: their pads too). */
    static std::vector<const char*> groups() {
        std::vector<const char*> g = {"game.", "copter."};
        if (ugh_sim_has_stage("passengers")) { g.push_back("pad."); g.push_back("passenger."); }
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

    void compare(const std::string& what, const Tick& cur, const Fields& expected, const std::vector<const char*>& groups, bool allKnown) {
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
                if (allKnown) { diffs += " " + name + " unknown (expected " + value + ")"; n++; }
                else r_.unknown[name]++;
            } else {
                r_.compared[name.substr(0, name.find('.'))]++;
            }
            if (it != actual.end() && it->second != value) {
                diffs += " " + name + " expected " + value + " got " + it->second;
                n++;
            }
        }
        if (n > 0) report(cur, what, std::to_string(n) + " fields:" + diffs);
    }
};

bool play(ugh_sim* sim, const std::string& path, Result& result) {
    std::ifstream in(path);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", path.c_str()); return false; }
    std::string line;
    if (!std::getline(in, line) || line != "UGR 0") { std::fprintf(stderr, "%s: not a UGR 0 file\n", path.c_str()); return false; }
    ugh_sim_reset(sim);
    Player player(sim, result);
    Fields state;
    std::vector<Tick> window;  // the previous and the current tick
    auto flush = [&]() {
        if (window.size() == 2) { player.step(window[0], window[1]); window.erase(window.begin()); }
    };
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || startsWith(line, "meta ")) continue;
        if (startsWith(line, "T ")) {
            flush();
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
    flush();
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: ugh_replay <ugh-sim.bin> <replay.ugr> ...\n");
        return 2;
    }
    char err[256];
    ugh_sim* sim = ugh_sim_create(argv[1], err, sizeof err);
    if (!sim) { std::fprintf(stderr, "%s\n", err); return 2; }
    bool ok = true;
    for (int i = 2; i < argc; i++) {
        Result r;
        if (!play(sim, argv[i], r)) { ok = false; continue; }
        std::string checked;
        for (const auto& [what, n] : r.checked) checked += " " + what + " " + std::to_string(n) + ",";
        std::string compared;
        for (const auto& [group, n] : r.compared) compared += " " + group + " " + std::to_string(n);
        std::printf("%s:%s skipped %lld, %lld mismatches; values compared:%s\n", argv[i], checked.c_str(), r.skipped,
                    r.mismatches, compared.c_str());
        for (const auto& line : r.reports) std::printf("  %s\n", line.c_str());
        if (!r.unknown.empty()) {
            std::string names;
            for (const auto& [name, n] : r.unknown) names += " " + name;
            std::printf("  not known after a level start:%s\n", names.c_str());
        }
        if (r.mismatches > 0) ok = false;
    }
    ugh_sim_destroy(sim);
    return ok ? 0 : 1;
}
