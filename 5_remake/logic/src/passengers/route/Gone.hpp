// The passenger state Gone.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Off the level: its route is done, or it drowned. */
class Gone : public RouteState {
public:
    static const Gone instance;

    const char* name() const override { return "Gone"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger&, const PassengerContext&) const override {}
};

}  // namespace ugh::passengers::route
