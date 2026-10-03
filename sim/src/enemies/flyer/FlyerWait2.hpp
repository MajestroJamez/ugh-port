// The enemy state FlyerWait2.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Screeches, then flies. */
class FlyerWait2 : public EnemyState {
public:
    static const FlyerWait2 instance;

    const char* name() const override { return "FlyerWait2"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
