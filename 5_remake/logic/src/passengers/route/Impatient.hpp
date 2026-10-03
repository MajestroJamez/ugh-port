// The passenger state Impatient.
#pragma once

#include "passengers/route/OnPickupPad.hpp"

namespace ugh::passengers::route {

/** Waves impatiently for a while, then calls the next copter with room on its pad, or waits again. */
class Impatient : public OnPickupPad {
public:
    static const Impatient instance;
    static constexpr int WAVE_TIME = 140;

    const char* name() const override { return "Impatient"; }
    void enter(RoutePassenger& passenger, const PassengerContext& context) const override;
    void stay(RoutePassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::route
