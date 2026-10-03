// A new attempt, and its caption fading in.
#pragma once

#include "game/phases/TimedPhase.hpp"

namespace ugh::game::phases {

/** A new attempt at the level starts (the level is loaded), and its caption fades in. */
class CaptionFadeIn : public TimedPhase {
public:
    /** A fade in or out of a caption: the palette in 65 steps, one per frame. */
    static constexpr int FADE_FRAMES = 65;

    CaptionFadeIn() : TimedPhase(FADE_FRAMES, PhaseId::CaptionWaitKey) {}

    GamePhase reported() const override { return GamePhase::Caption; }

protected:
    void begin(GameFlow& flow) override;
};

}  // namespace ugh::game::phases
