// The enemy state TreeInit.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Where the level load starts a tree. */
class TreeInit : public EnemyState {
public:
    static const TreeInit instance;

    const char* name() const override { return "TreeInit"; }
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
