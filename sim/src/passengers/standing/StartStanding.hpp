// The passenger state StartStanding.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Where the level load starts the standing passenger, and where it lands after a fall. */
class StartStanding : public PassengerState {
public:
    static const StartStanding instance;

    const char* name() const override { return "StartStanding"; }
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
