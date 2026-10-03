// The passenger state Gone.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Off the level: its route is done, it drowned, or it fell off the screen. */
class Gone : public PassengerState {
public:
    static const Gone instance;

    const char* name() const override { return "Gone"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
