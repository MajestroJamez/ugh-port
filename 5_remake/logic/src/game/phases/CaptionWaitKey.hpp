// The caption waits for a key.
#pragma once

#include "game/Phase.hpp"
#include "game/PhaseId.hpp"

namespace ugh::game::phases {

/** The caption stays until a key comes. */
class CaptionWaitKey : public Phase {
public:
    explicit CaptionWaitKey(PhaseId next) : next_(next) {}

    void enter(GameFlow& flow) override;
    void nextFrame(GameFlow& flow) override;
    GamePhase reported() const override { return GamePhase::Caption; }

private:
    PhaseId next_;
};

}  // namespace ugh::game::phases
