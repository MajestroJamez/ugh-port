// The walker's state Watching.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies::walker {

/** It looks at the copter that landed on its pad for a while, then charges; it walks on when the copter took off. */
class Watching : public WalkerState {
public:
    static const Watching instance;

    const char* name() const override { return "Watching"; }
    void enter(Walker& walker, const EnemyContext& context) const override;
    void update(Walker& walker, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::walker
