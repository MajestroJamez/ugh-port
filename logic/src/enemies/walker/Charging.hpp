// The walker's state Charging.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies::walker {

/** It charges at the copter on its pad, faster and faster; hitting it throws the copter into the air. */
class Charging : public WalkerState {
public:
    static const Charging instance;

    const char* name() const override { return "Charging"; }
    void enter(Walker& walker, const EnemyContext& context) const override;
    void update(Walker& walker, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::walker
