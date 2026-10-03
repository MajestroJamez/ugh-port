// The walker's state Placed.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies::walker {

/** Put on its pad (or back after a charge or a stun), it walks. */
class Placed : public WalkerState {
public:
    static const Placed instance;

    const char* name() const override { return "Placed"; }
    void update(Walker& walker, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::walker
