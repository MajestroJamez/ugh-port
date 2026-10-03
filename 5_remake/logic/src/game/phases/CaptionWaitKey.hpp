// The caption waits for a key.
#pragma once

#include "game/Phase.hpp"

namespace ugh::game::phases {

/** The caption stays until a key comes. */
class CaptionWaitKey : public Phase {
public:
    void enter(GameFlow& flow) override;
    void nextFrame(GameFlow& flow) override;
    GamePhase reported() const override { return GamePhase::Caption; }
};

}  // namespace ugh::game::phases
