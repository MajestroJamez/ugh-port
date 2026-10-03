// The enemy state Flying.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Flies across the screen at the height of its target's copter; touching that copter ends the attempt. */
class Flying : public EnemyState {
public:
    static const Flying instance;

    const char* name() const override { return "Flying"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
