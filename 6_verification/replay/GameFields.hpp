// The fields of the game.
#pragma once

#include "game/Game.hpp"
#include "replay/Fields.hpp"

namespace ugh::replay {

/** The game.* fields: phase, level, players, lives ...; the level's water, rain, energy and fade while it is loaded. */
class GameFields {
public:
    static void write(const game::Game& game, Fields& fields);
};

}  // namespace ugh::replay
