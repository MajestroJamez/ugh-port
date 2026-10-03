#include "game/phases/CaptionWaitKey.hpp"

#include "game/Game.hpp"
#include "game/GameFlow.hpp"

namespace ugh::game::phases {

/** A key held since before the caption does not count: the last scancode is taken now. */
void CaptionWaitKey::enter(GameFlow& flow) { flow.game().keyboard().readLastScancode(); }

void CaptionWaitKey::nextFrame(GameFlow& flow) {
    if (flow.game().keyboard().readLastScancode().changed) flow.goTo(PhaseId::CaptionFadeOut);
}

}  // namespace ugh::game::phases
