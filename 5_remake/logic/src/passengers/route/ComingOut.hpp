// The passenger state ComingOut.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Out of the door of the pickup pad, which it takes for itself; then it waits. */
class ComingOut : public RouteState {
public:
    static const ComingOut instance;

    const char* name() const override { return "ComingOut"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
