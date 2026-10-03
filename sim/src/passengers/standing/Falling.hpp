// The passenger state Falling.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Dropped from the copter: falls until it lands on a pad or leaves the screen; enemies are hit by it. */
class Falling : public PassengerState {
public:
    static const Falling instance;

    const char* name() const override { return "Falling"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
    bool fallsDown() const override { return true; }
};

}  // namespace ugh::passengers
