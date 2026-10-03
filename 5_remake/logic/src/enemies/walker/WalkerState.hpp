// A state of the walker.
#pragma once

#include "enemies/EnemyContext.hpp"
#include "state/State.hpp"

namespace ugh::enemies::walker {

class Walker;

/** A state of the walker: walking on its pad, watching a landed copter, charging at it, recovering, stunned. */
class WalkerState : public state::State<Walker, EnemyContext> {
protected:
    /** A standing passenger fell onto the walker: it is stunned; true when it was. */
    static bool stunnedByPassenger(Walker& walker, const EnemyContext& context);
};

}  // namespace ugh::enemies::walker
