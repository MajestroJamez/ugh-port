// The game: the one entry to the logic.
#pragma once

#include <cstdint>
#include <optional>

#include "bonuses/BonusSlots.hpp"
#include "data/GameData.hpp"
#include "events/Diagnostics.hpp"
#include "events/EventBroadcast.hpp"
#include "game/Cheats.hpp"
#include "game/GameFlow.hpp"
#include "game/GamePhase.hpp"
#include "game/GameResult.hpp"
#include "game/NewGameSettings.hpp"
#include "enemies/Enemies.hpp"
#include "input/PcKeyboard.hpp"
#include "passengers/Passengers.hpp"
#include "world/Level.hpp"
#include "world/PlayContext.hpp"
#include "world/Session.hpp"

namespace ugh::game {

/**
 * The game (Facade): a new game, the scancodes of the keyboard and one frame after another, until the game is over;
 * the state can be read (a renderer, the replays) and the events are reported to listeners. Nothing can be set from
 * outside but through Cheats (the test pilot of the replays).
 */
class Game {
public:
    explicit Game(const data::GameData& data);
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /** Events go to `listener` from now on. */
    void addListener(events::EventListener& listener) { events_.add(listener); }

    /** A new game; the first step starts it. */
    void newGame(const NewGameSettings& settings);

    /** A scancode from the keyboard, between two frames. */
    void scancode(uint8_t code);

    /** One frame (1/70 s). */
    GameResult step() { return flow_.step(); }

    /** The test pilot of the replays. */
    Cheats cheats() { return Cheats(*this); }

    // ------------------------------------------------------------ reading

    GamePhase phase() const { return flow_.phase(); }
    /** The session of the game; only after newGame(). */
    const world::Session& session() const { return *session_; }
    const world::Level& level() const { return level_; }
    const passengers::Passengers& passengers() const { return passengers_; }
    const enemies::Enemies& enemies() const { return enemies_; }
    const bonuses::BonusSlots& bonuses() const { return bonuses_; }
    /** A level is loaded: its world and its entities are there (caption, setup, play). */
    bool levelLoaded() const;
    events::Diagnostics& diagnostics() { return diagnostics_; }

    // ------------------------------------------------------------ for the phases of the flow

    /** The game starts: lives, multiplier, score. */
    void startGame();
    /** A new attempt at the current level. */
    void startAttempt();
    /** The level lists get their first update, then nothing is shown. */
    void beforePlay();
    /** One frame of the play. */
    void playFrame();
    /** The attempt is over (its fade-out reached black). */
    bool attemptOver() const { return level_.fade().over(); }
    /** The next level when the attempt finished the level, else a life less. */
    GameResult endAttempt();
    input::PcKeyboard& keyboard() { return keyboard_; }
    void report(const events::Event& event) { events_.onEvent(event); }

private:
    friend class Cheats;

    const data::GameData& data_;
    events::EventBroadcast events_;
    events::Diagnostics diagnostics_;
    std::optional<world::Session> session_;
    world::Level level_;
    passengers::Passengers passengers_;
    enemies::Enemies enemies_;
    bonuses::BonusSlots bonuses_;   // they stay from the end of an attempt until the play of the next one
    input::PcKeyboard keyboard_;
    GameFlow flow_;

    world::PlayContext context();
};

}  // namespace ugh::game
