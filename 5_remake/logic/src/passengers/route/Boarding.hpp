// The passenger state Boarding.
#pragma once

#include "passengers/route/OnPickupPad.hpp"

namespace ugh::passengers::route {

/** Walks to the landed copter and gets in; waves impatiently when the copter took off. */
class Boarding : public OnPickupPad {
public:
    static const Boarding instance;

    const char* name() const override { return "Boarding"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void stay(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
