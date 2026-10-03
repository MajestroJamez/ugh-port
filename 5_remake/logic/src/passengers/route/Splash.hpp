// The passenger state Splash.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Into the water: it goes under and floats up to the surface; its pickup pad is free again. */
class Splash : public RouteState {
public:
    static const Splash instance;

    const char* name() const override { return "Splash"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
