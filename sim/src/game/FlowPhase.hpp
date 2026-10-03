// A phase of the game flow.
#pragma once

namespace ugh::game {

class GameFlow;

/**
 * A phase of the game flow (State): a stretch of the original between waits for the vertical retrace, like the
 * fade-in of a level caption or the play of a level. The original waits for the retrace once per frame; a phase
 * does the work of the original from one wait to the next.
 */
class FlowPhase {
public:
    virtual ~FlowPhase() = default;

    /** The work at the start of the phase, up to its first wait (or the move to the next phase). */
    virtual void enter(GameFlow& flow) = 0;

    /** The retrace came: the work up to the next wait; GameFlow::moveTo when the phase is over. */
    virtual void afterRetrace(GameFlow& flow) = 0;
};

}  // namespace ugh::game
