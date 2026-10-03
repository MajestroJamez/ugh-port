#include "game/phases/CaptionFadeIn.hpp"

#include "game/Game.hpp"
#include "game/GameFlow.hpp"

namespace ugh::game::phases {

void CaptionFadeIn::begin(GameFlow& flow) {
    flow.game().startAttempt();
    flow.game().report({events::EventKind::LevelCaption});
}

}  // namespace ugh::game::phases
