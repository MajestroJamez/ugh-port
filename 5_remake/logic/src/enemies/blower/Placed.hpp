// The blower's state Placed.
#pragma once

#include "enemies/blower/BlowerState.hpp"

namespace ugh::enemies::blower {

/** Put into the level (or back after a stun), it blows. */
class Placed : public BlowerState {
public:
    static const Placed instance;

    const char* name() const override { return "Placed"; }
    void update(Blower& blower, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::blower
