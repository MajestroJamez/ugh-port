// A state of the flyer.
#pragma once

#include "enemies/EnemyContext.hpp"

namespace ugh::enemies::flyer {

class Flyer;

/** A state of the flyer (State): hidden, screeching, flying across the screen at its target, falling when hit. */
class FlyerState {
public:
    virtual ~FlyerState() = default;
    virtual const char* name() const = 0;
    virtual void enter(Flyer&, const EnemyContext&) const {}
    virtual void update(Flyer& flyer, const EnemyContext& context) const = 0;
};

}  // namespace ugh::enemies::flyer
