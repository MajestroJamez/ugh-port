#include "game/phases/CaptionFadeIn.hpp"

#include "game/GameFlow.hpp"

namespace ugh::game::phases {

void CaptionFadeIn::begin(GameFlow& flow) {
    flow.attempts().start();
    flow.attempts().report({events::EventKind::LevelCaption});
}

}  // namespace ugh::game::phases
