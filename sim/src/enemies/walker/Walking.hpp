// The enemy state Walking.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies {

/** Walks to and fro along its pad; watches a copter that lands there. */
class Walking : public WalkerState {
public:
    static const Walking instance;

    const char* name() const override { return "Walking"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
