// The enemy state Blowing.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Blows copters in front of it sideways, to and fro with the animation. */
class Blowing : public EnemyState {
public:
    static const Blowing instance;

    const char* name() const override { return "Blowing"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
