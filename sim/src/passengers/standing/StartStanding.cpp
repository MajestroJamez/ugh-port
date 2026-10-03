#include "passengers/standing/StartStanding.hpp"

#include "passengers/standing/Standing.hpp"

namespace ugh::passengers {

const StartStanding StartStanding::instance{};

/** 113b:1c0f */
void StartStanding::update(model::Passenger& passenger, model::Level& level) const {
    passenger.changeState(Standing::instance, level);
}

}  // namespace ugh::passengers
