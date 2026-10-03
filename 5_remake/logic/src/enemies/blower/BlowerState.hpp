// A state of the blower.
#pragma once

#include "enemies/EnemyContext.hpp"
#include "state/State.hpp"

namespace ugh::enemies::blower {

class Blower;

/** A state of the blower: blowing, or stunned. */
using BlowerState = state::State<Blower, EnemyContext>;

}  // namespace ugh::enemies::blower
