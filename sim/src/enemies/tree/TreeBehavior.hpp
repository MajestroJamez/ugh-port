// How the level load places a tree.
#pragma once

#include "enemies/EnemyBehavior.hpp"

namespace ugh::enemies {

/** The tree: on its pad, with the bonus items it drops. */
class TreeBehavior : public EnemyBehavior {
public:
    static const TreeBehavior instance;

    void place(model::Enemy& enemy, const data::EnemyPlacement& placement) const override;
};

}  // namespace ugh::enemies
