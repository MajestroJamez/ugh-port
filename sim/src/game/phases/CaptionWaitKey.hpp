// The caption waits for a key.
#pragma once

#include "game/FlowPhase.hpp"

namespace ugh::game::phases {

/** 113b:44db - Host.kt waitKey: the caption stays until the scancode changes (no joystick). */
class CaptionWaitKey : public FlowPhase {
public:
    explicit CaptionWaitKey(FlowPhase& next) : next_(next) {}

    void enter(GameFlow& flow) override;
    void afterRetrace(GameFlow& flow) override;

private:
    FlowPhase& next_;
};

}  // namespace ugh::game::phases
