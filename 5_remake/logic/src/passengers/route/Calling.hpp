// The passenger state Calling.
#pragma once

#include "passengers/route/OnPickupPad.hpp"

namespace ugh::passengers::route {

/** Calls the copter that landed on its pad (a bubble with the number of its destination), then walks to it. */
class Calling : public OnPickupPad {
public:
    static const Calling instance;
    /** How long a passenger calls before it walks to the copter. */
    static constexpr int CALL_TIME = 140;

    const char* name() const override { return "Calling"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void stay(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
