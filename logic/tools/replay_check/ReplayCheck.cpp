#include "ReplayCheck.hpp"

#include <cctype>
#include <cstdio>

#include "replay/StateWriter.hpp"

namespace ugh::tool {

bool ReplayCheck::run(const std::string& path) {
    ReplayFile file(path);
    if (!file.open()) {
        std::fprintf(stderr, "%s\n", file.error().c_str());
        return false;
    }
    game::Game game(data_);
    Tick tick, previous;
    bool started = false;
    while (file.next(tick)) {
        if (stopsAt(tick)) break;
        if (!started) {
            if (!start(game, tick)) return true;
            started = true;
        } else {
            apply(game, previous);
            game::GameResult result = game.step();
            if (result != game::GameResult::Continue) {
                report_.problem(tick.number, "the logic ended the game");
                return true;
            }
        }
        diagnostics(game, tick.number);
        report_.tick();
        if (!compare(game, tick)) return true;
        previous = tick;
    }
    if (!file.error().empty()) {
        std::fprintf(stderr, "%s\n", file.error().c_str());
        return false;
    }
    return true;
}

/** Tick 0: the settings of the new game. */
bool ReplayCheck::start(game::Game& game, const Tick& tick) {
    game::NewGameSettings settings;
    const replay::Fields& s = tick.state;
    auto value = [&](const char* name) -> std::string {
        auto it = s.find(name);
        return it == s.end() ? "" : it->second;
    };
    settings.players = std::stoi(value("game.players"));
    std::string difficulty = value("game.difficulty");
    settings.difficulty = difficulty == "easy" ? data::Difficulty::Easy
                          : difficulty == "hard" ? data::Difficulty::Hard
                                                 : data::Difficulty::Medium;
    settings.firstLevel = std::stoi(value("game.level"));
    std::string rng = value("game.rng");
    if (rng.size() != 16) {
        report_.problem(tick.number, "no game.rng at tick 0");
        return false;
    }
    for (int i = 0; i < 4; i++) settings.randomSeed[3 - i] = static_cast<uint16_t>(std::stoul(rng.substr(4 * i, 4), nullptr, 16));
    settings.rainFloorRow = std::stoi(value("game.rainFloor"));
    game.newGame(settings);
    return true;
}

bool ReplayCheck::compare(const game::Game& game, const Tick& tick) {
    replay::Fields actual = replay::StateWriter::write(game);
    std::string diffs;
    int n = 0;
    for (const auto& [name, expected] : tick.state) {
        if (!compared(name)) continue;
        auto it = actual.find(name);
        if (it == actual.end()) {
            diffs += "\n    " + name + " expected " + expected + ", missing";
            n++;
            continue;
        }
        report_.compared(name.substr(0, name.find('.')));
        if (it->second != expected) {
            diffs += "\n    " + name + " expected " + expected + ", got " + it->second;
            n++;
        }
    }
    for (const auto& [name, value] : actual) {
        if (compared(name) && !tick.state.count(name)) {
            diffs += "\n    " + name + " not expected, got " + value;
            n++;
        }
    }
    if (n == 0) return true;
    auto phase = tick.state.find("game.phase");
    std::string keys;
    for (const std::string& k : recent_) keys += " " + k;
    report_.problem(tick.number, "phase " + (phase == tick.state.end() ? std::string("?") : phase->second) + ", " +
                                     std::to_string(n) + " fields:" + diffs + "\n    scancodes of the last ticks:" + keys);
    return options_.continueAfterMismatch;
}

/** After the tick: the test pilot's interventions, then the scancodes. */
void ReplayCheck::apply(game::Game& game, const Tick& tick) {
    std::string keys = std::to_string(tick.number) + ":";
    for (int code : tick.scancodes) {
        game.scancode(static_cast<uint8_t>(code));
        char hex[4];
        std::snprintf(hex, sizeof hex, "%02x", code);
        keys += std::string(" ") + hex;
    }
    recent_.push_back(keys);
    if (recent_.size() > RECENT_TICKS) recent_.pop_front();
}

bool ReplayCheck::compared(const std::string& field) const {
    if (!options_.only.empty()) {
        bool in = false;
        for (const std::string& prefix : options_.only) in = in || field.rfind(prefix, 0) == 0;
        if (!in) return false;
    }
    std::string g = general(field);
    for (const std::string& skipped : options_.skip)
        if (g == skipped || field == skipped) return false;
    return true;
}

bool ReplayCheck::stopsAt(const Tick& tick) const {
    if (options_.untilField.empty()) return false;
    auto it = tick.state.find(options_.untilField);
    std::string value = it == tick.state.end() ? "~" : it->second;
    return options_.untilNot ? value != options_.untilValue : value == options_.untilValue;
}

void ReplayCheck::diagnostics(game::Game& game, long long tick) {
    for (const std::string& problem : game.diagnostics().take()) report_.problem(tick, "the logic: " + problem);
}

/** passenger.3.x -> passenger.N.x */
std::string ReplayCheck::general(const std::string& field) {
    std::string g;
    for (size_t i = 0; i < field.size(); i++) {
        if (std::isdigit(static_cast<unsigned char>(field[i])) && i > 0 && field[i - 1] == '.') {
            while (i + 1 < field.size() && std::isdigit(static_cast<unsigned char>(field[i + 1]))) i++;
            g += 'N';
        } else {
            g += field[i];
        }
    }
    return g;
}

}  // namespace ugh::tool
