// The passenger state Waiting.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Walks to the waiting spot of the pad and stands there; calls a copter with room that lands on the pad. */
class Waiting : public RouteState {
public:
    static const Waiting instance;

    const char* name() const override { return "Waiting"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
