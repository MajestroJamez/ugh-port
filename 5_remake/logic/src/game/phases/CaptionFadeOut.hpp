// The caption fades out.
#pragma once

#include "game/phases/CaptionFadeIn.hpp"
#include "game/phases/TimedPhase.hpp"

namespace ugh::game::phases {

/** The caption fades out. */
class CaptionFadeOut : public TimedPhase {
public:
    explicit CaptionFadeOut(PhaseId next) : TimedPhase(CaptionFadeIn::FADE_FRAMES, next) {}

    GamePhase reported() const override { return GamePhase::Caption; }
};

}  // namespace ugh::game::phases
