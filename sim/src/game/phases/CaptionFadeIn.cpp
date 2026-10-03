#include "game/phases/CaptionFadeIn.hpp"

#include "game/Game.hpp"
#include "game/GameFlow.hpp"
#include "game/LevelLoader.hpp"

namespace ugh::game::phases {

void CaptionFadeIn::begin(GameFlow& flow) {
    model::Level& level = flow.game().level();
    LevelLoader::startAttempt(level);
    levelWaterRow_ = level.water().row();
    level.water().setRow(Game::CAPTION_WATER_ROW);
    level.report({core::EventKind::LevelCaption});
}

}  // namespace ugh::game::phases
