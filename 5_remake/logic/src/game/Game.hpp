// The game: the one entry to the logic.
#pragma once

#include "bonuses/BonusSlots.hpp"
#include "data/GameData.hpp"
#include "enemies/Enemies.hpp"
#include "events/Diagnostics.hpp"
#include "events/EventBroadcast.hpp"
#include "game/GameFlow.hpp"
#include "game/GamePhase.hpp"
#include "game/GameResult.hpp"
#include "game/GameState.hpp"
#include "game/NewGameSettings.hpp"
#include "input/MenuKey.hpp"
#include "input/PlayerKey.hpp"
#include "passengers/Passengers.hpp"
#include "world/Level.hpp"
#include "world/session/Session.hpp"

namespace ugh::testing {
class TestPilot;   // the test pilot of the replays (5_remake/logic/testing)
}

namespace ugh::game {

/**
 * The game (Facade): a new game, the pilots' keys and the keys of the game loop, one frame after another, until the
 * game is over; the state can be read (a renderer, the replays) and the events are reported to listeners. Nothing can
 * be set from outside but by the test pilot of the replays (`testing::TestPilot`); the diagnostics are taken (and so
 * cleared) from outside.
 */
class Game {
public:
    explicit Game(const data::GameData& data);
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /** Events go to `listener` from now on. */
    void addListener(events::EventListener& listener) { events_.add(listener); }

    /** A new game; the first step starts it. False (and no game) when a setting is out of range. */
    bool newGame(const NewGameSettings& settings);

    /** A pilot's key pressed or released, between two frames. */
    void key(int player, input::PlayerKey key, bool pressed) { state_.level.copters()[player].controls().set(key, pressed); }
    /** A key event the game loop sees (Esc, P, any other), between two frames. */
    void menuKey(input::MenuKey key) { state_.menu.receive(key); }

    /** One frame (1/70 s). */
    GameResult step() { return flow_.step(); }

    // ------------------------------------------------------------ reading

    GamePhase phase() const { return flow_.phase(); }
    /** The session of the game; only after newGame(). */
    const world::session::Session& session() const { return *state_.session; }
    const world::Level& level() const { return state_.level; }
    const passengers::Passengers& passengers() const { return state_.passengers; }
    const enemies::Enemies& enemies() const { return state_.enemies; }
    const bonuses::BonusSlots& bonuses() const { return state_.bonuses; }
    /** A level is loaded: its world and its entities are there (caption, setup, play). */
    bool levelLoaded() const;
    /** What the logic does not support, so far: not only read - whoever takes the problems clears them. */
    events::Diagnostics& diagnostics() { return diagnostics_; }

private:
    friend class testing::TestPilot;   // the one way into the state: the copters, the energy and the lives

    const data::GameData& data_;
    events::EventBroadcast events_;
    events::Diagnostics diagnostics_;
    GameState state_;
    GameFlow flow_;
};

}  // namespace ugh::game
