// The passenger state Riding.
#pragma once

#include "passengers/route/RouteState.hpp"
#include "world/copter/Copter.hpp"

namespace ugh::passengers::route {

/** In the copter: the fare drops, the time for a quick delivery runs out, until the copter lands at the target pad. */
class Riding : public RouteState {
public:
    static const Riding instance;

    /** The passenger gets into `copter`. */
    static void board(RoutePassenger& passenger, world::copter::Copter& copter, const PassengerContext& context);

    const char* name() const override { return "Riding"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
