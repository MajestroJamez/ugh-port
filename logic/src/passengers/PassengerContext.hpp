// What a passenger's update gets.
#pragma once

#include "bonuses/BonusSlots.hpp"
#include "world/PlayContext.hpp"

namespace ugh::passengers {

/** What the update of a passenger gets: the play, and the bonus items (a quick delivery drops one). */
struct PassengerContext {
    const world::PlayContext& play;
    bonuses::BonusSlots& bonuses;
};

}  // namespace ugh::passengers
