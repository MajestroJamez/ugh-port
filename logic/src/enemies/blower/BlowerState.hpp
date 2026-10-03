// A state of the blower.
#pragma once

#include "enemies/EnemyContext.hpp"

namespace ugh::enemies::blower {

class Blower;

/** A state of the blower (State): blowing, or stunned. */
class BlowerState {
public:
    virtual ~BlowerState() = default;
    virtual const char* name() const = 0;
    virtual void enter(Blower&, const EnemyContext&) const {}
    virtual void update(Blower& blower, const EnemyContext& context) const = 0;
};

}  // namespace ugh::enemies::blower
