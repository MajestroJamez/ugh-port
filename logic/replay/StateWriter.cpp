#include "replay/StateWriter.hpp"

#include "replay/CopterFields.hpp"
#include "replay/GameFields.hpp"
#include "replay/PadFields.hpp"

namespace ugh::replay {

Fields StateWriter::write(const game::Game& game) {
    Fields fields;
    GameFields::write(game, fields);
    if (!game.levelLoaded()) return fields;
    const world::Level& level = game.level();
    for (int player = 0; player < level.copterCount(); player++) CopterFields::write(level.copter(player), player, fields);
    for (int i = 0; i < level.padCount(); i++) PadFields::write(level.pad(i), i, fields);
    return fields;
}

}  // namespace ugh::replay
