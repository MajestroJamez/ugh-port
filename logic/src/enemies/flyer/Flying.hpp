// The flyer's state Flying.
#pragma once

#include "enemies/flyer/FlyerState.hpp"

namespace ugh::enemies::flyer {

/** It flies across the screen at its target's height; touching its target's copter ends the attempt. */
class Flying : public FlyerState {
public:
    static const Flying instance;

    const char* name() const override { return "Flying"; }
    void enter(Flyer& flyer, const EnemyContext& context) const override;
    void update(Flyer& flyer, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::flyer
