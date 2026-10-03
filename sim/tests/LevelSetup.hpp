// A game at the start of a level attempt, for the tests.
#pragma once

#include <string>

#include "core/EventQueue.hpp"
#include "core/Word.hpp"
#include "game/Game.hpp"
#include "model/Level.hpp"

namespace ugh::test {

/** A game at the start of a level attempt: one player, level `number` loaded, the copters in the air. */
class LevelSetup {
public:
    explicit LevelSetup(int number = 0);

    game::Game game;
    core::EventQueue events;
    model::Level& level;

    std::string passengerState(int i) const;

    /** Runs the passengers' state machines until passenger i is in the state; false after `frames` frames. */
    bool runPassengersUntil(int i, const std::string& state, int frames = 2000);

    /** Lands copter c on a pad, at x. */
    void land(int c, int pad, core::Word x);

    /** Puts copter c over the middle of pad 1, `height` px above it. */
    void hover(int c, int height);

    bool reported(core::EventKind kind) const;
};

}  // namespace ugh::test
