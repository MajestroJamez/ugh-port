// A state of the walker.
#pragma once

#include "enemies/EnemyContext.hpp"

namespace ugh::enemies::walker {

class Walker;

/** A state of the walker (State): walking on its pad, watching a landed copter, charging at it, recovering, stunned. */
class WalkerState {
public:
    virtual ~WalkerState() = default;
    virtual const char* name() const = 0;
    virtual void enter(Walker&, const EnemyContext&) const {}
    virtual void update(Walker& walker, const EnemyContext& context) const = 0;

protected:
    /** A standing passenger fell onto the walker: it is stunned; true when it was. */
    static bool stunnedByPassenger(Walker& walker, const EnemyContext& context);
};

}  // namespace ugh::enemies::walker
