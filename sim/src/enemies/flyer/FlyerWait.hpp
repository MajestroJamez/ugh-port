// The enemy state FlyerWait.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Hidden until its start delay is over. */
class FlyerWait : public EnemyState {
public:
    static const FlyerWait instance;

    const char* name() const override { return "FlyerWait"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
