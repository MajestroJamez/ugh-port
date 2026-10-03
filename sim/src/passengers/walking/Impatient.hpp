// The passenger state Impatient.
#pragma once

#include "passengers/walking/OnPickupPad.hpp"

namespace ugh::passengers {

/** The copter left without the passenger, or came full: it waves impatiently for a while. */
class Impatient : public OnPickupPad {
public:
    static const Impatient instance;

    const char* name() const override { return "Impatient"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
