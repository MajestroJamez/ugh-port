// The passenger state Boarding.
#pragma once

#include "passengers/walking/OnPickupPad.hpp"

namespace ugh::passengers {

/** Walks to the landed copter and gets in. */
class Boarding : public OnPickupPad {
public:
    static const Boarding instance;

    const char* name() const override { return "Boarding"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
