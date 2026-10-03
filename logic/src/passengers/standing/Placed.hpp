// The standing passenger's state Placed.
#pragma once

#include "passengers/standing/StandingState.hpp"

namespace ugh::passengers::standing {

/** Put on its pad (by the level, or by its landing), it starts to stand. */
class Placed : public StandingState {
public:
    static const Placed instance;

    const char* name() const override { return "Placed"; }
    void update(StandingPassenger& passenger, const PassengerContext& context) const override;
};

}  // namespace ugh::passengers::standing
