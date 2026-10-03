// The passenger state SwimCalling.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** A copter floats on the water: the swimmer calls it for a while. */
class SwimCalling : public PassengerState {
public:
    static const SwimCalling instance;

    const char* name() const override { return "SwimCalling"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
