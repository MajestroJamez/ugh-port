// The enemy state FlyerFalling.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Hit by a passenger: falls out of the sky, faster and faster. */
class FlyerFalling : public EnemyState {
public:
    static const FlyerFalling instance;

    const char* name() const override { return "FlyerFalling"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
