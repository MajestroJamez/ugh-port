// The passenger state NextStop.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Where the level load starts a walking passenger, and where it goes after each stop: the next stop of its route, or off the level when the route is done. */
class NextStop : public PassengerState {
public:
    static const NextStop instance;

    const char* name() const override { return "NextStop"; }
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
