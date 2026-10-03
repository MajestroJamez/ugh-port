#include "game/phases/TimedPhase.hpp"

#include "game/GameFlow.hpp"

namespace ugh::game::phases {

void TimedPhase::enter(GameFlow& flow) {
    begin(flow);
    left_.start(retraces_);
}

void TimedPhase::afterRetrace(GameFlow& flow) {
    if (left_.tick()) flow.moveTo(next_);
}

}  // namespace ugh::game::phases
