// What the test pilot of the replays may do.
#pragma once

#include <optional>

#include "units/Fixed.hpp"
#include "units/Int16.hpp"
#include "units/Speed.hpp"

namespace ugh::game {

class Game;

/**
 * The interventions of the test pilot that recorded the golden replays (its I lines): it puts a copter anywhere and
 * keeps the energy and the lives up. Nothing else of the state can be set from outside.
 */
class Cheats {
public:
    explicit Cheats(Game& game) : game_(game) {}

    void placeCopter(int player, units::Fixed x, units::Fixed y, units::Int16 pixelX, units::Int16 pixelY, units::Speed vx,
                     units::Speed vy, std::optional<int> landedPad);
    void setEnergy(units::Int16 energy);
    void setLives(int lives);

private:
    Game& game_;
};

}  // namespace ugh::game
