// The passenger state SwimWaving.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** The copter left: the swimmer waves for a while, then sinks. */
class SwimWaving : public PassengerState {
public:
    static const SwimWaving instance;

    const char* name() const override { return "SwimWaving"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
