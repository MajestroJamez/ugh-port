// The phases of the game flow.
#pragma once

namespace ugh::game {

/** The phases of the game flow, in their order. */
enum class PhaseId {
    BlackBeforeCaption,
    CaptionFadeIn,
    CaptionWaitKey,
    CaptionFadeOut,
    BlackBeforePlay,
    Playing,
    Count   // not a phase: how many there are
};

}  // namespace ugh::game
