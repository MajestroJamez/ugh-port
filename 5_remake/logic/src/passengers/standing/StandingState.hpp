// A state of the standing passenger.
#pragma once

#include "passengers/PassengerContext.hpp"

namespace ugh::passengers::standing {

class StandingPassenger;

/**
 * A state of the standing passenger (State): it waits on its pad, hangs below the copter that picked it up, falls
 * when the pilot lets it go, and stands again where it lands.
 */
class StandingState {
public:
    virtual ~StandingState() = default;

    /** The name of the state (the replays). */
    virtual const char* name() const = 0;
    /** What the passenger does when it gets into the state. */
    virtual void enter(StandingPassenger&, const PassengerContext&) const {}
    /** One frame in the state. */
    virtual void update(StandingPassenger& passenger, const PassengerContext& context) const = 0;
    /** It falls in this state (enemies are hit by it). */
    virtual bool falls() const { return false; }
};

}  // namespace ugh::passengers::standing
