// The flyer's state Hidden.
#pragma once

#include "enemies/flyer/FlyerState.hpp"

namespace ugh::enemies::flyer {

/** Off the screen for its start delay. */
class Hidden : public FlyerState {
public:
    static const Hidden instance;

    const char* name() const override { return "Hidden"; }
    void enter(Flyer& flyer, const EnemyContext& context) const override;
    void update(Flyer& flyer, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::flyer
