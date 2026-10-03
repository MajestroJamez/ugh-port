#include "replay/StateWriter.hpp"

#include "replay/GameFields.hpp"

namespace ugh::replay {

Fields StateWriter::write(const game::Game& game) {
    Fields fields;
    GameFields::write(game, fields);
    return fields;
}

}  // namespace ugh::replay
