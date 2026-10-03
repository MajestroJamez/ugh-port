#include "game/Cheats.hpp"

#include "game/Game.hpp"

namespace ugh::game {

void Cheats::placeCopter(int player, units::Fixed x, units::Fixed y, units::Int16 pixelX, units::Int16 pixelY,
                         units::Speed vx, units::Speed vy, std::optional<int> landedPad) {
    game_.level_.copter(player).placeByTestPilot(x, y, pixelX, pixelY, vx, vy, landedPad);
}

void Cheats::setEnergy(units::Int16 energy) { game_.level_.energy().setByTestPilot(energy); }

void Cheats::setLives(int lives) { game_.session_->setLivesByTestPilot(lives); }

}  // namespace ugh::game
