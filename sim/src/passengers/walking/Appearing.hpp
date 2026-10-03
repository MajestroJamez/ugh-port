// The passenger state Appearing.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** Comes out of the door of the pickup pad; the pad has a passenger waiting now. */
class Appearing : public PassengerState {
public:
    static const Appearing instance;

    const char* name() const override { return "Appearing"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
