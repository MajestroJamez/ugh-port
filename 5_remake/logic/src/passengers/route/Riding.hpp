// The passenger state Riding.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** In the copter: the fare drops, the time for a quick delivery runs out, until the copter lands at the target pad. */
class Riding : public RouteState {
public:
    static const Riding instance;
    /** Frames of riding that still earn a bonus item. */
    static constexpr int QUICK_DELIVERY_TIME = 200;

    /** The passenger gets into the copter of `player`. */
    static void board(RoutePassenger& passenger, int player, const PassengerContext& context);

    const char* name() const override { return "Riding"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
