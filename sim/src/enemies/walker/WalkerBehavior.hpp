// How the level load places a walker.
#pragma once

#include "enemies/EnemyBehavior.hpp"

namespace ugh::enemies {

/** The walker: on its pad, with its speed, facing left. */
class WalkerBehavior : public EnemyBehavior {
public:
    static const WalkerBehavior instance;

    void place(model::Enemy& enemy, const data::EnemyPlacement& placement) const override;
};

}  // namespace ugh::enemies
