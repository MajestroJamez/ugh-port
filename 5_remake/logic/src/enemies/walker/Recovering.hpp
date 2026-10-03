// The walker's state Recovering.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies::walker {

/** After a charge (the copter left, or it was thrown): one run of the animation, then it walks again. */
class Recovering : public WalkerState {
public:
    static const Recovering instance;

    const char* name() const override { return "Recovering"; }
    void enter(Walker& walker, const EnemyContext& context) const override;
    void update(Walker& walker, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::walker
