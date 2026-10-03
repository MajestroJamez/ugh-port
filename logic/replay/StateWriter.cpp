#include "replay/StateWriter.hpp"

#include "replay/BonusFields.hpp"
#include "replay/CopterFields.hpp"
#include "replay/GameFields.hpp"
#include "replay/PadFields.hpp"
#include "replay/PassengerFields.hpp"

namespace ugh::replay {

Fields StateWriter::write(const game::Game& game) {
    Fields fields;
    GameFields::write(game, fields);
    if (!game.levelLoaded()) return fields;
    const world::Level& level = game.level();
    for (int player = 0; player < level.copterCount(); player++) CopterFields::write(level.copter(player), player, fields);
    for (int i = 0; i < level.padCount(); i++) PadFields::write(level.pad(i), i, fields);
    PassengerFields passengers(fields);
    for (int i = 0; i < game.passengers().count(); i++) game.passengers()[i].accept(passengers);
    for (int slot = 0; slot < bonuses::BonusSlots::SLOTS; slot++)
        if (game.bonuses()[slot]) BonusFields::write(*game.bonuses()[slot], fields);
    return fields;
}

}  // namespace ugh::replay
