// The enemy state Stunned.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Hit by a passenger: stunned for a while. */
class Stunned : public EnemyState {
public:
    static const Stunned instance;

    const char* name() const override { return "Stunned"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
