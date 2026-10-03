#include "passengers/route/Swim.hpp"

namespace ugh::passengers::route {

namespace {

using units::Speed;

// in 1/64 Fixed per frame, per frame
constexpr Speed GRAVITY = Speed::fromRaw(39);        // through the air, and sinking
constexpr Speed WATER_BRAKE = Speed::fromRaw(373);   // going down in the water
constexpr Speed BUOYANCY = Speed::fromRaw(92);       // coming up
constexpr Speed MAX_SPEED = Speed::fromRaw(6144);

}  // namespace

Speed Swim::splash(bool aboveSurface) {
    if (aboveSurface) speed_ += GRAVITY;
    else if (speed_ > Speed()) speed_ -= WATER_BRAKE;
    else speed_ -= BUOYANCY;
    speed_ = speed_.clamped(MAX_SPEED);
    return speed_;
}

Speed Swim::sink() {
    speed_ += GRAVITY;
    return speed_;
}

}  // namespace ugh::passengers::route
