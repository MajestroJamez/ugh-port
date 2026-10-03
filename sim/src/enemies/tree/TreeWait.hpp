// The enemy state TreeWait.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** Rests after a passenger hit it; then sways again, or stays still when its bonus items are all gone. */
class TreeWait : public EnemyState {
public:
    static const TreeWait instance;

    const char* name() const override { return "TreeWait"; }
    void enter(model::Enemy& enemy, model::Level& level) const override;
    void update(model::Enemy& enemy, model::Level& level) const override;
};

}  // namespace ugh::enemies
