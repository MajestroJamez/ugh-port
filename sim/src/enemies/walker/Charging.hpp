// The enemy state Charging.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies {

/** Charges at the copter, faster and faster; hitting it throws the copter into the air. */
class Charging : public WalkerState {
public:
    static const Charging instance;

    const char* name() const override { return "Charging"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
