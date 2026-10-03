// The enemy state BlowerWait.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Hit by a passenger: stunned for a while. */
class BlowerWait : public EnemyState {
public:
    static const BlowerWait instance;

    const char* name() const override { return "BlowerWait"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
