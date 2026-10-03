// A state of the flyer.
#pragma once

#include "enemies/EnemyContext.hpp"
#include "state/State.hpp"

namespace ugh::enemies::flyer {

class Flyer;

/** A state of the flyer: hidden, screeching, flying across the screen at its target, falling when hit. */
using FlyerState = state::State<Flyer, EnemyContext>;

}  // namespace ugh::enemies::flyer
