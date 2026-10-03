// What an enemy's update gets.
#pragma once

#include "bonuses/BonusSlots.hpp"
#include "passengers/Passengers.hpp"
#include "world/PlayContext.hpp"

namespace ugh::enemies {

/** What the update of an enemy gets: the play, the passengers (one may fall onto it), the bonus items (a tree drops one). */
struct EnemyContext : world::PlayContext {
    passengers::Passengers& passengers;
    bonuses::BonusSlots& bonuses;
};

}  // namespace ugh::enemies
