// What the test pilot of the replays may do.
#pragma once

#include <optional>

#include "game/Game.hpp"
#include "units/Fixed.hpp"
#include "units/Speed.hpp"
#include "world/Copter.hpp"

namespace ugh::testing {

/**
 * The test pilot that recorded the golden replays (their I lines): it puts a copter anywhere and keeps the energy and
 * the lives up. Not part of the logic: the unit tests and the replay check (6_verification) use it, the logic only
 * names it as a friend of what it sets (Copter, Energy, Session, Game).
 */
class TestPilot {
public:
    explicit TestPilot(game::Game& game) : game_(game) {}

    /** The copter of `player` at x, y with its pixel position, speed and the index of the pad it stands on. */
    void placeCopter(int player, units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx,
                     units::Speed vy, std::optional<int> landedPad);
    void setEnergy(int energy);
    void setLives(int lives);

    /** The same for a copter without a game (the tests of the physics). */
    static void placeCopter(world::Copter& copter, units::Fixed x, units::Fixed y, int pixelX, int pixelY,
                            units::Speed vx, units::Speed vy, std::optional<int> landedPad);

private:
    game::Game& game_;
};

}  // namespace ugh::testing
