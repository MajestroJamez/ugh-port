// The passenger state WalkingAway.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Delivered: pays and walks from the copter to the door of the target pad. */
class WalkingAway : public PassengerState {
public:
    static const WalkingAway instance;

    const char* name() const override { return "WalkingAway"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
