// The passenger state NextStop.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Between two stops: the next stop starts (it waits behind the door), or the route is done. */
class NextStop : public RouteState {
public:
    static const NextStop instance;

    const char* name() const override { return "NextStop"; }
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
