// Bonus items (bonuses.cpp): dropped by trees and for quick deliveries, collected by touching them.
#pragma once

#include <vector>

#include "data.hpp"
#include "fixed.hpp"

namespace ugh {

class Game;

/** A state of a bonus item; the original keeps the address of its handler (2d83). */
struct BonusState {
    const char* name;   // as in the golden replays
    void (*update)(Game& game, int slot);
};

/** Every state, for the replay projection. */
const std::vector<const BonusState*>& bonusStates();

/**
 * 113b:2b96 - Bonuses.kt bonusSpawn: drops a bonus item with its middle / bottom at x, y, flying sideways with vx
 * and up with its kind's lift plus `lift`.
 */
void dropBonus(Game& game, const BonusKind& kind, Fixed x, Fixed y, int16_t vx, int16_t lift);

}  // namespace ugh
