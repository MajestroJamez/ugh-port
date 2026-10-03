// The passenger state Arriving.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Hidden behind the door of the pickup pad until its delay is over and nobody else waits on the pad. */
class Arriving : public PassengerState {
public:
    static const Arriving instance;

    const char* name() const override { return "Arriving"; }
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
