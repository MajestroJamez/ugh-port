#include "game/phases/CaptionWaitKey.hpp"

#include "game/GameFlow.hpp"

namespace ugh::game::phases {

/** A key that came before the caption does not count. */
void CaptionWaitKey::enter(GameFlow& flow) { flow.attempts().menu().takeArrived(); }

void CaptionWaitKey::nextFrame(GameFlow& flow) {
    if (flow.attempts().menu().takeArrived()) flow.goTo(next_);
}

}  // namespace ugh::game::phases
