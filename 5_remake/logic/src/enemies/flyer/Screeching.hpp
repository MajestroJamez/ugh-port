// The flyer's state Screeching.
#pragma once

#include "enemies/flyer/FlyerState.hpp"

namespace ugh::enemies::flyer {

/** Its screech is heard a while before it flies in. */
class Screeching : public FlyerState {
public:
    static const Screeching instance;

    const char* name() const override { return "Screeching"; }
    void enter(Flyer& flyer, const EnemyContext& context) const override;
    void update(Flyer& flyer, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::flyer
