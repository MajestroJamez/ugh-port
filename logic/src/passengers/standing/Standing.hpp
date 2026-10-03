// The standing passenger's state Standing.
#pragma once

#include "passengers/standing/StandingState.hpp"

namespace ugh::passengers::standing {

/** It waits for a copter with room to touch it. */
class Standing : public StandingState {
public:
    static const Standing instance;

    const char* name() const override { return "Standing"; }
    void update(StandingPassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::standing
