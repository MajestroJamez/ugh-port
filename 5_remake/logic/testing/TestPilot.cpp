#include "testing/TestPilot.hpp"

namespace ugh::testing {

void TestPilot::placeCopter(int player, units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx,
                            units::Speed vy, std::optional<int> landedPad) {
    placeCopter(game_.state_.level.copters()[player], x, y, pixelX, pixelY, vx, vy, landedPad);
}

void TestPilot::setEnergy(int energy) { game_.state_.level.energy().value_ = energy; }

void TestPilot::setLives(int lives) { game_.state_.session->lives_ = lives; }

void TestPilot::placeCopter(world::Copter& copter, units::Fixed x, units::Fixed y, int pixelX, int pixelY,
                            units::Speed vx, units::Speed vy, std::optional<int> landedPad) {
    world::Motion& motion = copter;
    motion.x_ = x;
    motion.y_ = y;
    motion.pixelX_ = pixelX;
    motion.pixelY_ = pixelY;
    motion.setSpeed(vx, vy);
    copter.landedPad_ = landedPad;
}

}  // namespace ugh::testing
