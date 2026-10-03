#include "game/phases/Playing.hpp"

#include "core/Audit.hpp"
#include "game/Game.hpp"
#include "game/GameFlow.hpp"

namespace ugh::game::phases {

/** The game palette, the tiles and the background page are drawn here; then the lists get their first update. */
void Playing::enter(GameFlow& flow) {
    model::Level& level = flow.game().level();
    core::audit::Scope scope("playing enter");
    level.updateEnemies();
    level.updatePassengers();
    level.hideSprites();
    if (level.fade().over()) flow.endAttempt();
}

/** 113b:4e36: the palette at the fade position, then the frame. */
void Playing::afterRetrace(GameFlow& flow) {
    flow.game().playFrame();
    if (flow.game().level().fade().over()) flow.endAttempt();
}

}  // namespace ugh::game::phases
