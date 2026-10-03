// The enemy state Watching.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies {

/** Looks at the copter on its pad for a while, then charges. */
class Watching : public WalkerState {
public:
    static const Watching instance;

    const char* name() const override { return "Watching"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
