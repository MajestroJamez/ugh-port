// What an attempt at a level starts from.
#pragma once

#include <array>
#include <cstdint>

#include "data/Difficulty.hpp"
#include "input/MenuKey.hpp"
#include "world/session/RandomNumbers.hpp"

namespace ugh::game {

/**
 * What an attempt at a level starts from: everything of the game that lasts into it from the attempt before (or from
 * the new game) - the session (players, difficulty, the level, lives, score, multiplier, random numbers), the row the
 * rain stops at, how hard each pilot last worked his rotor (it turns on with it while the level fades in) and the last
 * key the game loop saw (Esc gives up). The rest of the attempt is loaded from the level's definition, so a game
 * resumed from it (`Game::resume`) plays the attempt exactly as the game it came from - the replays of a level
 * (`record/`) start so. The bonus items of the attempt before stay through the caption and are cleared before the play:
 * nothing of them reaches it.
 */
struct AttemptStart {
    int players = 1;
    data::Difficulty difficulty = data::Difficulty::Medium;
    int level = 0;   // from 0 in the order of the mode
    int lives = 0;
    uint32_t points = 0;
    int multiplier = 1;
    world::session::RandomNumbers::Words random{};
    int rainFloorRow = 0;
    std::array<int, 2> effort{};   // by player; 0 for a copter the mode has not
    input::MenuKey lastMenuKey = input::MenuKey::Other;

    bool operator==(const AttemptStart&) const = default;
};

}  // namespace ugh::game
