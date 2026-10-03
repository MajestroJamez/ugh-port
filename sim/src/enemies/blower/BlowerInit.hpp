// The enemy state BlowerInit.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Where the level load starts a blower, and where it goes back to blowing. */
class BlowerInit : public EnemyState {
public:
    static const BlowerInit instance;

    const char* name() const override { return "BlowerInit"; }
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
