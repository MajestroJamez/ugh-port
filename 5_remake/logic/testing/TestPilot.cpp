#include "testing/TestPilot.hpp"

namespace ugh::testing {

void TestPilot::placeCopter(int player, units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx,
                            units::Speed vy, std::optional<int> landedPad) {
    world::Level& level = game_.state_.level;
    placeCopter(level.copters()[player], x, y, pixelX, pixelY, vx, vy, landedPad ? &level.pad(*landedPad) : nullptr);
}

void TestPilot::setEnergy(int energy) { game_.state_.level.energy() = world::Energy(energy); }

void TestPilot::setLives(int lives) { game_.state_.session->lives() = world::session::Lives(lives); }

void TestPilot::placeCopter(world::copter::Copter& copter, units::Fixed x, units::Fixed y, int pixelX, int pixelY,
                            units::Speed vx, units::Speed vy, const world::scenery::Pad* landedOn) {
    if (landedOn) copter.land(*landedOn);
    else copter.takeOff();
    copter.motion() = world::copter::Motion(x, y, pixelX, pixelY, vx, vy);
}

}  // namespace ugh::testing
