// The walker's state Walking.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies::walker {

/** It walks along its pad, a step per animation frame, turning at the ends; it watches a copter that lands there. */
class Walking : public WalkerState {
public:
    static const Walking instance;

    const char* name() const override { return "Walking"; }
    void enter(Walker& walker, const EnemyContext& context) const override;
    void update(Walker& walker, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::walker
