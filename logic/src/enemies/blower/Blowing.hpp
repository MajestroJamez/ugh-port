// The blower's state Blowing.
#pragma once

#include "enemies/blower/BlowerState.hpp"

namespace ugh::enemies::blower {

/** It blows: copters in front of it are pushed sideways, one way in the first frames of its animation, back after. */
class Blowing : public BlowerState {
public:
    static const Blowing instance;

    const char* name() const override { return "Blowing"; }
    void enter(Blower& blower, const EnemyContext& context) const override;
    void update(Blower& blower, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::blower
