// The enemy state WalkerInit.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Where the level load starts a walker, and where it goes back to walking. */
class WalkerInit : public EnemyState {
public:
    static const WalkerInit instance;

    const char* name() const override { return "WalkerInit"; }
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
