#include "game/phases/TimedPhase.hpp"

#include "game/GameFlow.hpp"

namespace ugh::game::phases {

void TimedPhase::enter(GameFlow& flow) {
    begin(flow);
    left_.start(frames_);
}

void TimedPhase::nextFrame(GameFlow& flow) {
    if (left_.tick()) flow.goTo(next_);
}

}  // namespace ugh::game::phases
