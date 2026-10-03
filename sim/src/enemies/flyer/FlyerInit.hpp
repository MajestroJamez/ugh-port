// The enemy state FlyerInit.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Where the level load starts a flyer, and where it starts again after each flight. */
class FlyerInit : public EnemyState {
public:
    static const FlyerInit instance;

    const char* name() const override { return "FlyerInit"; }
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
