// A phase of the game flow.
#pragma once

#include "game/GamePhase.hpp"

namespace ugh::game {

class GameFlow;

/**
 * A phase of the game flow (State): a stretch of frames such as the fade-in of a caption or the play of a level. The
 * original waits for the vertical retrace once per frame; a phase does what the original does between two waits.
 */
class Phase {
public:
    virtual ~Phase() = default;

    /** The work at the start of the phase, up to its first wait. */
    virtual void enter(GameFlow& flow) = 0;

    /** The next frame: the work up to the next wait; GameFlow::goTo when the phase is over. */
    virtual void nextFrame(GameFlow& flow) = 0;

    /** Where the game is in this phase. */
    virtual GamePhase reported() const = 0;
};

}  // namespace ugh::game
