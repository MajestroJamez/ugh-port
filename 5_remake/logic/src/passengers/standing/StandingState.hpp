// A state of the standing passenger.
#pragma once

#include "passengers/PassengerContext.hpp"
#include "state/State.hpp"

namespace ugh::passengers::standing {

class StandingPassenger;

/**
 * A state of the standing passenger: it waits on its pad, hangs below the copter that picked it up, falls
 * when the pilot lets it go, and stands again where it lands.
 */
class StandingState : public state::State<StandingPassenger, PassengerContext> {
public:
    /** It falls in this state (enemies are hit by it). */
    virtual bool falls() const { return false; }
};

}  // namespace ugh::passengers::standing
