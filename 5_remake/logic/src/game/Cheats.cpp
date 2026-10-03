#include "game/Cheats.hpp"

#include "game/Game.hpp"

namespace ugh::game {

void Cheats::placeCopter(int player, units::Fixed x, units::Fixed y, int pixelX, int pixelY,
                         units::Speed vx, units::Speed vy, std::optional<int> landedPad) {
    game_.level_.copters()[player].placeByTestPilot(x, y, pixelX, pixelY, vx, vy, landedPad);
}

void Cheats::setEnergy(int energy) { game_.level_.energy().setByTestPilot(energy); }

void Cheats::setLives(int lives) { game_.session_->setLivesByTestPilot(lives); }

}  // namespace ugh::game
