// The enemy state Inactive.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** A tree without bonus items: nothing more happens. */
class Inactive : public EnemyState {
public:
    static const Inactive instance;

    const char* name() const override { return "Inactive"; }
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
