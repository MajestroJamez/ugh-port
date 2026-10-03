// The passenger state SwimBoarding.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Swims to the copter on the water and gets in. */
class SwimBoarding : public PassengerState {
public:
    static const SwimBoarding instance;

    const char* name() const override { return "SwimBoarding"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
