// The passenger state Standing.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Stands on its pad until a copter with room for it touches it. */
class Standing : public PassengerState {
public:
    static const Standing instance;

    const char* name() const override { return "Standing"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
