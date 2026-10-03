// The passenger state Hanging.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Hangs below the copter that picked it up until its pilot presses fire. */
class Hanging : public PassengerState {
public:
    static const Hanging instance;

    const char* name() const override { return "Hanging"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
