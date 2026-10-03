// The caption fades out.
#pragma once

#include "game/phases/CaptionFadeIn.hpp"
#include "game/phases/TimedPhase.hpp"

namespace ugh::game::phases {

/**
 * 113b:0664 (end) - Level.kt levelCaption: the level's water row back, and the caption fading out
 * (113b:4e28 - Host.kt fadeOut).
 */
class CaptionFadeOut : public TimedPhase {
public:
    CaptionFadeOut(const CaptionFadeIn& caption, FlowPhase& next)
        : TimedPhase(CaptionFadeIn::FADE_RETRACES, next), caption_(caption) {}

protected:
    void begin(GameFlow& flow) override;

private:
    const CaptionFadeIn& caption_;
};

}  // namespace ugh::game::phases
