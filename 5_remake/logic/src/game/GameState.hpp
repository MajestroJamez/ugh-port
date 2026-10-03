// What changes during a game.
#pragma once

#include <optional>

#include "bonuses/BonusSlots.hpp"
#include "data/Rules.hpp"
#include "enemies/Enemies.hpp"
#include "game/NewGameSettings.hpp"
#include "input/MenuInput.hpp"
#include "passengers/Passengers.hpp"
#include "world/Level.hpp"
#include "world/Session.hpp"

namespace ugh::game {

/**
 * Everything of a game that changes from frame to frame: the session (lives, score, random numbers), the level being
 * played, its passengers, enemies and bonus items, and the keys of the game loop. `Game` owns it; the game flow
 * (`Attempts`) works on it.
 */
struct GameState {
    std::optional<world::Session> session;   // from newGame() on
    world::Level level;
    passengers::Passengers passengers;
    enemies::Enemies enemies;
    bonuses::BonusSlots bonuses;   // they stay from the end of an attempt until the play of the next one
    input::MenuInput menu;

    /** A new game with `settings`: nothing of the last game stays. */
    void reset(const data::Rules& rules, const NewGameSettings& settings);
};

}  // namespace ugh::game
