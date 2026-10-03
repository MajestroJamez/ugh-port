// The passenger state Waiting.
#pragma once

#include "passengers/walking/OnPickupPad.hpp"

namespace ugh::passengers {

/** Walks to the waiting spot of the pickup pad and waits there for a copter with room for it. */
class Waiting : public OnPickupPad {
public:
    static const Waiting instance;

    const char* name() const override { return "Waiting"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
