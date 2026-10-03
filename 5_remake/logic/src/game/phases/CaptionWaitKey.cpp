#include "game/phases/CaptionWaitKey.hpp"

#include "game/Game.hpp"
#include "game/GameFlow.hpp"

namespace ugh::game::phases {

/** A key that came before the caption does not count. */
void CaptionWaitKey::enter(GameFlow& flow) { flow.game().menu().takeArrived(); }

void CaptionWaitKey::nextFrame(GameFlow& flow) {
    if (flow.game().menu().takeArrived()) flow.goTo(PhaseId::CaptionFadeOut);
}

}  // namespace ugh::game::phases
