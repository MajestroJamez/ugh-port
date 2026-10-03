// The enemy state TreeSwaying.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Sways; a passenger falling onto it bounces off and shakes a bonus item out of it. */
class TreeSwaying : public EnemyState {
public:
    static const TreeSwaying instance;

    const char* name() const override { return "Tree"; }
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
