// The passenger state Sinking.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** It drowns: it sinks until it is below the bottom of the screen. */
class Sinking : public RouteState {
public:
    static const Sinking instance;

    const char* name() const override { return "Sinking"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
