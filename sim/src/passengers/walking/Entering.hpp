// The passenger state Entering.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Goes in at the door of the target pad; then the next stop of the route. */
class Entering : public PassengerState {
public:
    static const Entering instance;

    const char* name() const override { return "Entering"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
