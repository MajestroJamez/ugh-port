// The standing passenger's state Falling.
#pragma once

#include "passengers/standing/StandingState.hpp"

namespace ugh::passengers::standing {

/** Let go: it falls with the copter's speed; it stands where it lands on a pad, or is gone off the screen. */
class Falling : public StandingState {
public:
    static const Falling instance;

    const char* name() const override { return "Falling"; }
    void enter(StandingPassenger& passenger, const PassengerContext& context) const override;
    void update(StandingPassenger& passenger, const PassengerContext& context) const override;
    bool falls() const override { return true; }
};

}  // namespace ugh::passengers::standing
