// The passenger state SwimWaving.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Waves at the copter that left for a while, then sinks. */
class SwimWaving : public RouteState {
public:
    static const SwimWaving instance;

    const char* name() const override { return "SwimWaving"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
