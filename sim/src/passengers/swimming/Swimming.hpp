// The passenger state Swimming.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Swims; a copter floating still nearby rescues it, else it sinks when its time is up. */
class Swimming : public PassengerState {
public:
    static const Swimming instance;

    const char* name() const override { return "Swimming"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
