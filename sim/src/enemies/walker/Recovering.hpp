// The enemy state Recovering.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies {

/** After charging: one run of the animation, then walking again. */
class Recovering : public WalkerState {
public:
    static const Recovering instance;

    const char* name() const override { return "Recovering"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
