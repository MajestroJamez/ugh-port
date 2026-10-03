// The passenger state Riding.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** On board of a copter until it lands on the target pad; the fare drops meanwhile. */
class Riding : public PassengerState {
public:
    static const Riding instance;

    /** 113b:19fb / 19e0 - the passenger got to the copter (on a pad or on the water) and gets in. */
    static void board(model::Passenger& passenger, int copter, model::Level& level);

    const char* name() const override { return "Riding"; }
    void enter(model::Passenger& passenger, model::Level& level) const override;
    void update(model::Passenger& passenger, model::Level& level) const override;
};

}  // namespace ugh::passengers
