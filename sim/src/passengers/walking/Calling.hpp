// The passenger state Calling.
#pragma once

#include "passengers/walking/OnPickupPad.hpp"

namespace ugh::passengers {

/** A copter landed on the pickup pad: the passenger shows where it wants to go and calls it for a while. */
class Calling : public OnPickupPad {
public:
    static const Calling instance;

    const char* name() const override { return "Calling"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
