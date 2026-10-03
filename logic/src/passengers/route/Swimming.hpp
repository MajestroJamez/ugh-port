// The passenger state Swimming.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Afloat for the swim time of its kind; a copter floating still on the water rescues it, else it sinks. */
class Swimming : public RouteState {
public:
    static const Swimming instance;

    const char* name() const override { return "Swimming"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
