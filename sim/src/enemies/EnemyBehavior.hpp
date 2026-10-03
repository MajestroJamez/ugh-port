// What differs between the kinds of enemies besides their states (Strategy).
#pragma once

#include "data/EnemyKind.hpp"
#include "data/EnemyPlacement.hpp"
#include "model/Enemy.hpp"

namespace ugh::enemies {

/**
 * How the level load puts an enemy of a kind into its slot (113b:3b21) and in which state it starts. A kind sets
 * only the values its entry in the level list has; the rest stays from the slot's earlier use.
 */
class EnemyBehavior {
public:
    virtual ~EnemyBehavior() = default;

    virtual void place(model::Enemy& enemy, const data::EnemyPlacement& placement) const = 0;

    /** The behavior of a kind. */
    static const EnemyBehavior& of(data::EnemyKind::Type type);
};

}  // namespace ugh::enemies
