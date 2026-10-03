// What a new game starts with.
#pragma once

#include "data/Difficulty.hpp"
#include "world/RandomNumbers.hpp"

namespace ugh::game {

/**
 * What a new game starts with: what the menu chose, and two values the screens before the game leave behind in the
 * original - the state of the random numbers and the row where raindrops start again (the water of the demo).
 */
struct NewGameSettings {
    int players = 1;   // 2: team mode
    data::Difficulty difficulty = data::Difficulty::Medium;
    int firstLevel = 0;   // from 0, in the order of the mode (a password starts later)
    world::RandomNumbers::Words randomSeed{};
    int rainFloorRow = 180;
};

}  // namespace ugh::game
