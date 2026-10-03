// The passenger state BehindDoor.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Behind the door of the pickup pad until the stop's delay is over and nobody waits on the pad. */
class BehindDoor : public RouteState {
public:
    static const BehindDoor instance;

    const char* name() const override { return "BehindDoor"; }
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
