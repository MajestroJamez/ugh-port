// What the test pilot of the replays may do.
#pragma once

#include <optional>

#include "game/Game.hpp"
#include "units/Fixed.hpp"
#include "units/Speed.hpp"
#include "world/Copter.hpp"
#include "world/Pad.hpp"

namespace ugh::testing {

/**
 * The test pilot that recorded the golden replays (their I lines): it puts a copter anywhere and keeps the energy and
 * the lives up. Not part of the logic: the unit tests and the replay check (6_verification) use it. It reaches the
 * state of a game as the one friend of the facade `game::Game`; the rest it does through the public operations of
 * the world.
 */
class TestPilot {
public:
    explicit TestPilot(game::Game& game) : game_(game) {}

    /**
     * The copter of `player` at x, y with its pixel position (as the probe sees it) and speed, standing on the pad
     * with index `landedPad` (the replays name it so) or in the air.
     */
    void placeCopter(int player, units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx,
                     units::Speed vy, std::optional<int> landedPad);
    void setEnergy(int energy);
    void setLives(int lives);

    /** The same for a copter without a game (the tests of the physics): standing on `landedOn`, or in the air. */
    static void placeCopter(world::Copter& copter, units::Fixed x, units::Fixed y, int pixelX, int pixelY,
                            units::Speed vx, units::Speed vy, const world::Pad* landedOn);

private:
    game::Game& game_;
};

}  // namespace ugh::testing
