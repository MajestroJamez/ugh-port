// The passenger state WalkingToDoor.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Delivered: it gets out, pays, and walks from the copter to the door of the target pad. */
class WalkingToDoor : public RouteState {
public:
    static const WalkingToDoor instance;

    const char* name() const override { return "WalkingToDoor"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
