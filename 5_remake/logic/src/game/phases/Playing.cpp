#include "game/phases/Playing.hpp"

#include "game/GameFlow.hpp"

namespace ugh::game::phases {

void Playing::enter(GameFlow& flow) {
    flow.attempts().beforePlay();
    if (flow.attempts().over()) flow.endAttempt();
}

void Playing::nextFrame(GameFlow& flow) {
    flow.attempts().playFrame();
    if (flow.attempts().over()) flow.endAttempt();
}

}  // namespace ugh::game::phases
