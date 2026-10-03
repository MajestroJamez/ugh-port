// The passenger state Calling.
#pragma once

#include "passengers/route/OnPickupPad.hpp"

namespace ugh::passengers::route {

/**
 * Calls the copter that landed on its pad (a bubble with the number of its destination) for a while
 * (`PickupWait::CALL_TIME`), then walks to it.
 */
class Calling : public OnPickupPad {
public:
    static const Calling instance;

    const char* name() const override { return "Calling"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void stay(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
