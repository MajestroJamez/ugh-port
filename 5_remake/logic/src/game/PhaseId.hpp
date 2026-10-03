// The phases of the game flow.
#pragma once

namespace ugh::game {

/** The phases of the game flow, in their order. */
enum class PhaseId { BlackBeforeCaption, CaptionFadeIn, CaptionWaitKey, CaptionFadeOut, BlackBeforePlay, Playing };

}  // namespace ugh::game
