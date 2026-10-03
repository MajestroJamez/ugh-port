// The passenger state Boarding.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Walks to the landed copter and gets in; waves impatiently when the copter took off. */
class Boarding : public RouteState {
public:
    static const Boarding instance;

    const char* name() const override { return "Boarding"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
