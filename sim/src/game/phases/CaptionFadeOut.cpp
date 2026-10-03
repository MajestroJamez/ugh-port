#include "game/phases/CaptionFadeOut.hpp"

#include "game/Game.hpp"
#include "game/GameFlow.hpp"

namespace ugh::game::phases {

void CaptionFadeOut::begin(GameFlow& flow) { flow.game().level().water().setRow(caption_.levelWaterRow()); }

}  // namespace ugh::game::phases
