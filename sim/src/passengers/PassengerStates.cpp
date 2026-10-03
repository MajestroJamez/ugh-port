#include "passengers/PassengerStates.hpp"

#include "passengers/standing/Falling.hpp"
#include "passengers/standing/Hanging.hpp"
#include "passengers/standing/StartStanding.hpp"
#include "passengers/standing/Standing.hpp"
#include "passengers/swimming/Sinking.hpp"
#include "passengers/swimming/Splash.hpp"
#include "passengers/swimming/SwimBoarding.hpp"
#include "passengers/swimming/SwimCalling.hpp"
#include "passengers/swimming/SwimWaving.hpp"
#include "passengers/swimming/Swimming.hpp"
#include "passengers/walking/Appearing.hpp"
#include "passengers/walking/Arriving.hpp"
#include "passengers/walking/Boarding.hpp"
#include "passengers/walking/Calling.hpp"
#include "passengers/walking/Entering.hpp"
#include "passengers/Gone.hpp"
#include "passengers/walking/Impatient.hpp"
#include "passengers/walking/NextStop.hpp"
#include "passengers/walking/Riding.hpp"
#include "passengers/walking/Waiting.hpp"
#include "passengers/walking/WalkingAway.hpp"

namespace ugh::passengers {

const std::vector<const PassengerState*>& PassengerStates::all() {
    static const std::vector<const PassengerState*> states = {
        &NextStop::instance,     &Arriving::instance,      &Appearing::instance,    &Waiting::instance,
        &Calling::instance,      &Impatient::instance,     &Boarding::instance,     &Riding::instance,
        &WalkingAway::instance,  &Entering::instance,      &Gone::instance,         &StartStanding::instance,
        &Standing::instance,     &Hanging::instance,       &Falling::instance,      &Splash::instance,
        &Sinking::instance,      &Swimming::instance,      &SwimCalling::instance,  &SwimWaving::instance,
        &SwimBoarding::instance};
    return states;
}

}  // namespace ugh::passengers
