// The passenger state Sinking.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Drowns: sinks until it is off the bottom of the screen. */
class Sinking : public PassengerState {
public:
    static const Sinking instance;

    const char* name() const override { return "Sinking"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
