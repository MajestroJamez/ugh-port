// What a passenger on its pickup pad watches for.
#pragma once

#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

/** The checks of the states on the pickup pad: the water up to the knees, a copter flying into the passenger. */
class OnPickupPad {
public:
    /** The water rose to its knees: it is in the water. */
    static bool fellIntoWater(RoutePassenger& passenger, const PassengerContext& context);
    /** A copter in the air touched it: it is knocked into the water. */
    static bool knockedIntoWater(RoutePassenger& passenger, const PassengerContext& context);

private:
    static void intoWater(RoutePassenger& passenger, const PassengerContext& context);
};

}  // namespace ugh::passengers::route
