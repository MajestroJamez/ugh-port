// The caption fades out.
#pragma once

#include "game/phases/CaptionFadeIn.hpp"
#include "game/phases/TimedPhase.hpp"

namespace ugh::game::phases {

/** The caption fades out. */
class CaptionFadeOut : public TimedPhase {
public:
    CaptionFadeOut() : TimedPhase(CaptionFadeIn::FADE_FRAMES, PhaseId::BlackBeforePlay) {}

    GamePhase reported() const override { return GamePhase::Caption; }
};

}  // namespace ugh::game::phases
