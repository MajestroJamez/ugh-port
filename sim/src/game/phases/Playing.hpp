// The play of a level attempt.
#pragma once

#include "game/FlowPhase.hpp"

namespace ugh::game::phases {

/**
 * 113b:3d66 (end) and 0c7d .. 0fa4 - Level.kt levelSetup, GameFlow.kt playLevel: the first update of the level
 * lists, then a play frame per retrace until the fade-out at the end of the attempt is over (GameFlow::endAttempt).
 */
class Playing : public FlowPhase {
public:
    void enter(GameFlow& flow) override;
    void afterRetrace(GameFlow& flow) override;
};

}  // namespace ugh::game::phases
