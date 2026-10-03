// The state of the game as replay fields.
#pragma once

#include "game/Game.hpp"
#include "replay/Fields.hpp"

namespace ugh::replay {

/**
 * The semantic state of the game as the fields of the replays "UGR 1" (verify/.../replay/SemanticProjection.kt; the
 * table in re/notes/rewrite-design.md, chap. 9). It only reads the game. A field is written exactly when the game
 * defines it, with the same rules as the Kotlin projection: the replay check compares the set of fields too.
 */
class StateWriter {
public:
    static Fields write(const game::Game& game);
};

}  // namespace ugh::replay
