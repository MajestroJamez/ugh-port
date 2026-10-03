#include "game/phases/CaptionWaitKey.hpp"

#include "game/Game.hpp"
#include "game/GameFlow.hpp"

namespace ugh::game::phases {

/** The scancode now, so that a key held since before the caption does not count. */
void CaptionWaitKey::enter(GameFlow& flow) { flow.game().keyboard().read(); }

void CaptionWaitKey::afterRetrace(GameFlow& flow) {
    if (flow.game().keyboard().read().changed) flow.moveTo(next_);
}

}  // namespace ugh::game::phases
