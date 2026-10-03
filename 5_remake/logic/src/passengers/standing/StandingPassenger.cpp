#include "passengers/standing/StandingPassenger.hpp"

#include "passengers/standing/Placed.hpp"

namespace ugh::passengers::standing {

namespace {

using units::Fixed;

// a falling passenger hits an enemy with this point (from its top left corner) inside the enemy's box (from the
// enemy's top left corner), in pixels
constexpr units::Int16 HIT_POINT_X = 12, HIT_POINT_Y = 8, ENEMY_WIDTH = 38, ENEMY_HEIGHT = 28;

}  // namespace

StandingPassenger::StandingPassenger(int index, const data::StandingPassengerPlacement& placement)
    : Passenger(index), StateMachine(Placed::instance), kind_(&placement.kind()) {
    x_ = placement.x();
    y_ = placement.y();
}

void StandingPassenger::update(const PassengerContext& context) { updateState(context); }

void StandingPassenger::accept(PassengerVisitor& visitor) const { visitor.visit(*this); }

bool StandingPassenger::fallsOnto(Fixed x, Fixed y) const {
    if (!state().falls() || fallSpeed_ < 0) return false;
    Fixed hitY = y_ + Fixed::fromPixels(HIT_POINT_Y), hitX = x_ + Fixed::fromPixels(HIT_POINT_X);
    return hitY >= y && hitY - Fixed::fromPixels(ENEMY_HEIGHT) <= y && hitX >= x && hitX - Fixed::fromPixels(ENEMY_WIDTH) <= x;
}

}  // namespace ugh::passengers::standing
