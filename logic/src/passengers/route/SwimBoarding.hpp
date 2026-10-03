// The passenger state SwimBoarding.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Swims to the copter on the water and gets in; waves when the copter is gone or full. */
class SwimBoarding : public RouteState {
public:
    static const SwimBoarding instance;

    const char* name() const override { return "SwimBoarding"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
