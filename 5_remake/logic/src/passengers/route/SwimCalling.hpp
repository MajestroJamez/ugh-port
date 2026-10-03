// The passenger state SwimCalling.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Calls the copter on the water for a while, then swims to it; waves when it left. */
class SwimCalling : public RouteState {
public:
    static const SwimCalling instance;

    const char* name() const override { return "SwimCalling"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
