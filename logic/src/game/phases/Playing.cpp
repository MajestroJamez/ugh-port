#include "game/phases/Playing.hpp"

#include "game/Game.hpp"
#include "game/GameFlow.hpp"

namespace ugh::game::phases {

void Playing::enter(GameFlow& flow) {
    flow.game().beforePlay();
    if (flow.game().attemptOver()) flow.endAttempt();
}

void Playing::nextFrame(GameFlow& flow) {
    flow.game().playFrame();
    if (flow.game().attemptOver()) flow.endAttempt();
}

}  // namespace ugh::game::phases
