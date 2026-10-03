// How the level load places a flyer.
#pragma once

#include "enemies/EnemyBehavior.hpp"

namespace ugh::enemies {

/** The flyer: its start delay and speed; its first flight goes for player 0. */
class FlyerBehavior : public EnemyBehavior {
public:
    static const FlyerBehavior instance;

    void place(model::Enemy& enemy, const data::EnemyPlacement& placement) const override;
};

}  // namespace ugh::enemies
