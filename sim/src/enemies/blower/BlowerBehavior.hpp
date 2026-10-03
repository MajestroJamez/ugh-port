// How the level load places a blower.
#pragma once

#include "enemies/EnemyBehavior.hpp"

namespace ugh::enemies {

/** The blower: where it stands. */
class BlowerBehavior : public EnemyBehavior {
public:
    static const BlowerBehavior instance;

    void place(model::Enemy& enemy, const data::EnemyPlacement& placement) const override;
};

}  // namespace ugh::enemies
