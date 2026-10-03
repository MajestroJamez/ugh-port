// The passenger state Splash.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** In the water: goes under and floats up to the surface. */
class Splash : public PassengerState {
public:
    static const Splash instance;

    const char* name() const override { return "Splash"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
