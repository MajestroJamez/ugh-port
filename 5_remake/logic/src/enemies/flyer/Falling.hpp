// The flyer's state Falling.
#pragma once

#include "enemies/flyer/FlyerState.hpp"

namespace ugh::enemies::flyer {

/** A passenger hit it: it falls, faster and faster, until it is off the screen; then it starts again. */
class Falling : public FlyerState {
public:
    static const Falling instance;

    const char* name() const override { return "Falling"; }
    void enter(Flyer& flyer, const EnemyContext& context) const override;
    void update(Flyer& flyer, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::flyer
