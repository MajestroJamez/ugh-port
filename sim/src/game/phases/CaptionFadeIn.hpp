// A new attempt at a level, and its caption fading in.
#pragma once

#include "core/Word.hpp"
#include "game/phases/TimedPhase.hpp"

namespace ugh::game::phases {

/**
 * 113b:3d66 + 0664 - Level.kt levelSetup and levelCaption: a new attempt at the level (the level loaded), and its
 * caption ("LEVEL nn", the text and the password) fading in (113b:4e19 - Host.kt fadeIn). The caption screen moves
 * the water row; the level's own row is kept for CaptionFadeOut.
 */
class CaptionFadeIn : public TimedPhase {
public:
    /** 113b:4e19 / 4e28 - a fade in or out: the palette steps 0, 4 .. 0x100 (Fade::FULL), one per retrace. */
    static constexpr int FADE_RETRACES = 65;

    explicit CaptionFadeIn(FlowPhase& next) : TimedPhase(FADE_RETRACES, next) {}

    /** The water row of the level, while the caption screen shows its own. */
    core::Word levelWaterRow() const { return levelWaterRow_; }

protected:
    void begin(GameFlow& flow) override;

private:
    core::Word levelWaterRow_;
};

}  // namespace ugh::game::phases
