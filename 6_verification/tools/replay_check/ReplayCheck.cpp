#include "ReplayCheck.hpp"

#include <charconv>
#include <cstdio>

#include "replay/StateWriter.hpp"
#include "testing/TestPilot.hpp"

namespace ugh::tool {

bool ReplayCheck::run(const std::string& path) {
    ReplayFile file(path);
    if (!file.open()) {
        std::fprintf(stderr, "%s\n", file.error().c_str());
        return false;
    }
    game::Game game(data_);
    keyboard::PcKeyboard keyboard(keys_);
    Tick tick, previous;
    bool started = false;
    while (file.next(tick)) {
        if (!started) {
            if (!start(game, tick)) return true;
            started = true;
        } else {
            apply(game, keyboard, previous);
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
    std::string difficulty = value("game.difficulty");
    settings.difficulty = difficulty == "easy" ? data::Difficulty::Easy
                          : difficulty == "hard" ? data::Difficulty::Hard
                                                 : data::Difficulty::Medium;
    std::string rng = value("game.rng");
    bool ok = number(value("game.players"), settings.players) && number(value("game.level"), settings.firstLevel) &&
              number(value("game.rainFloor"), settings.rainFloorRow) && rng.size() == 16;
    for (int i = 0; ok && i < 4; i++) {
        int word = 0;
        ok = number(rng.substr(4 * i, 4), word, 16);
        settings.randomSeed[3 - i] = static_cast<uint16_t>(word);
    }
    if (!ok || !game.newGame(settings)) {
        report_.problem(tick.number, "tick 0 has no valid game.players, game.level, game.rainFloor and game.rng");
        return false;
    }
    return true;
}

bool ReplayCheck::compare(const game::Game& game, const Tick& tick) {
    replay::Fields actual = replay::StateWriter::write(game);
    std::string diffs;
    int n = 0;
    for (const auto& [name, expected] : tick.state) {
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
        if (!tick.state.count(name)) {
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
void ReplayCheck::apply(game::Game& game, keyboard::PcKeyboard& keyboard, const Tick& tick) {
    if (!tick.inject.empty()) intervene(game, tick);
    std::string keys = std::to_string(tick.number) + ":";
    for (int code : tick.scancodes) {
        keyboard.deliver(static_cast<uint8_t>(code), game);
        char hex[4];
        std::snprintf(hex, sizeof hex, "%02x", code);
        keys += std::string(" ") + hex;
    }
    keyboard.beforeFrame(game);
    recent_.push_back(keys);
    if (recent_.size() > RECENT_TICKS) recent_.pop_front();
}

/** The I line through the test pilot: a copter put somewhere (the fields not in the line stay), energy, lives. */
void ReplayCheck::intervene(game::Game& game, const Tick& tick) {
    testing::TestPilot pilot(game);
    for (int player = 0; player < game.level().copters().count(); player++) {
        std::string c = "copter." + std::to_string(player) + ".";
        const world::Copter& copter = game.level().copters()[player];
        bool moved = false;
        auto value = [&](const char* field, int current) {
            auto it = tick.inject.find(c + field);
            if (it == tick.inject.end()) return current;
            moved = true;
            int v = current;
            if (!number(it->second, v)) report_.problem(tick.number, "a bad value " + it->first + "=" + it->second);
            return v;
        };
        units::Fixed x = units::Fixed::fromRaw(value("x", copter.motion().x().raw()));
        units::Fixed y = units::Fixed::fromRaw(value("y", copter.motion().y().raw()));
        int pixelX = value("pixelX", copter.motion().pixelX()), pixelY = value("pixelY", copter.motion().pixelY());
        units::Speed vx = units::Speed::fromRaw(value("vx", copter.motion().speedX().raw()));
        units::Speed vy = units::Speed::fromRaw(value("vy", copter.motion().speedY().raw()));
        std::optional<int> pad;
        if (copter.landedPad()) pad = copter.landedPad()->index();
        auto landed = tick.inject.find(c + "landedPad");
        if (landed != tick.inject.end()) {
            moved = true;
            int v = 0;
            pad = landed->second != "none" && number(landed->second, v) ? std::optional<int>(v) : std::nullopt;
        }
        if (moved) pilot.placeCopter(player, x, y, pixelX, pixelY, vx, vy, pad);
    }
    for (const auto& [field, value] : tick.inject) {
        int v = 0;
        bool known = field == "game.energy" || field == "game.lives";
        if (known && !number(value, v)) report_.problem(tick.number, "a bad value " + field + "=" + value);
        else if (field == "game.energy") pilot.setEnergy(v);
        else if (field == "game.lives") pilot.setLives(v);
        else if (field.rfind("copter.", 0) != 0) report_.problem(tick.number, "an intervention the logic does not allow: " + field);
    }
}

bool ReplayCheck::number(const std::string& text, int& out, int base) {
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), out, base);
    return error == std::errc() && end == text.data() + text.size() && !text.empty();
}

void ReplayCheck::diagnostics(game::Game& game, long long tick) {
    for (const std::string& problem : game.diagnostics().take()) report_.problem(tick, "the logic: " + problem);
}

}  // namespace ugh::tool
