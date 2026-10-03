// The passenger state GoingIn.
#pragma once

#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/** Into the door of the target pad; then the next stop of its route. */
class GoingIn : public RouteState {
public:
    static const GoingIn instance;

    const char* name() const override { return "GoingIn"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void update(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
