#include "passengers/standing/StandingPassenger.hpp"

#include "passengers/standing/Placed.hpp"

namespace ugh::passengers::standing {

namespace {

using units::Fixed;

// a falling passenger hits an enemy with this point (from its top left corner) inside the enemy's box (from the
// enemy's top left corner), in pixels
constexpr int HIT_POINT_X = 12, HIT_POINT_Y = 8, ENEMY_WIDTH = 38, ENEMY_HEIGHT = 28;

}  // namespace

StandingPassenger::StandingPassenger(int index, const data::levels::StandingPassengerPlacement& placement)
    : Passenger(index, placement.x, placement.y), StateMachine(Placed::instance), kind_(placement.kind) {}

void StandingPassenger::update(const PassengerContext& context) { updateState(context); }

void StandingPassenger::accept(PassengerVisitor& visitor) const { visitor.visit(*this); }

bool StandingPassenger::fallsOnto(Fixed enemyX, Fixed enemyY) const {
    if (!state().falls() || fallSpeed_ < Fixed()) return false;
    Fixed hitY = y() + Fixed::fromPixels(HIT_POINT_Y), hitX = x() + Fixed::fromPixels(HIT_POINT_X);
    return hitY >= enemyY && hitY - Fixed::fromPixels(ENEMY_HEIGHT) <= enemyY && hitX >= enemyX &&
           hitX - Fixed::fromPixels(ENEMY_WIDTH) <= enemyX;
}

}  // namespace ugh::passengers::standing
