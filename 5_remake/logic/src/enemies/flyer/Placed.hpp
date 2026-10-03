// The flyer's state Placed.
#pragma once

#include "enemies/flyer/FlyerState.hpp"

namespace ugh::enemies::flyer {

/** Put into the level (or back after a flight), it hides. */
class Placed : public FlyerState {
public:
    static const Placed instance;

    const char* name() const override { return "Placed"; }
    void update(Flyer& flyer, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::flyer
