// The standing passenger's state Hanging.
#pragma once

#include "passengers/standing/StandingState.hpp"

namespace ugh::passengers::standing {

/** Below the copter that picked it up, until its pilot lets it go (fire). */
class Hanging : public StandingState {
public:
    static const Hanging instance;

    const char* name() const override { return "Hanging"; }
    void enter(StandingPassenger& passenger, const PassengerContext& context) const override;
    void update(StandingPassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::standing
