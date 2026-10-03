// A state of a bonus item.
#pragma once

#include "bonuses/BonusItem.hpp"
#include "state/State.hpp"
#include "world/PlayContext.hpp"

namespace ugh::bonuses {

/** A state of a bonus item: falling, lying on a pad. */
using BonusState = state::State<BonusItem, world::PlayContext>;

}  // namespace ugh::bonuses
