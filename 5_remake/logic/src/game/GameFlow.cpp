#include "game/GameFlow.hpp"

#include "game/Game.hpp"
#include "game/phases/BlackScreen.hpp"
#include "game/phases/CaptionFadeIn.hpp"
#include "game/phases/CaptionFadeOut.hpp"
#include "game/phases/CaptionWaitKey.hpp"
#include "game/phases/Playing.hpp"

namespace ugh::game {

GameFlow::GameFlow(Game& game) : game_(game) {
    phases_[static_cast<int>(PhaseId::BlackBeforeCaption)] =
        std::make_unique<phases::BlackScreen>(PhaseId::CaptionFadeIn, GamePhase::BetweenLevels);
    phases_[static_cast<int>(PhaseId::CaptionFadeIn)] = std::make_unique<phases::CaptionFadeIn>();
    phases_[static_cast<int>(PhaseId::CaptionWaitKey)] = std::make_unique<phases::CaptionWaitKey>();
    phases_[static_cast<int>(PhaseId::CaptionFadeOut)] = std::make_unique<phases::CaptionFadeOut>();
    phases_[static_cast<int>(PhaseId::BlackBeforePlay)] =
        std::make_unique<phases::BlackScreen>(PhaseId::Playing, GamePhase::Setup);
    phases_[static_cast<int>(PhaseId::Playing)] = std::make_unique<phases::Playing>();
}

void GameFlow::restart() {
    current_ = nullptr;
    result_ = GameResult::Continue;
}

GameResult GameFlow::step() {
    if (result_ != GameResult::Continue) return result_;
    if (current_) {
        current_->nextFrame(*this);
    } else {
        game_.startGame();
        goTo(PhaseId::BlackBeforeCaption);
    }
    return result_;
}

void GameFlow::goTo(PhaseId next) {
    current_ = phases_[static_cast<int>(next)].get();
    current_->enter(*this);
}

void GameFlow::endAttempt() {
    GameResult end = game_.endAttempt();
    if (end == GameResult::Continue) {
        goTo(PhaseId::CaptionFadeIn);
    } else {
        current_ = nullptr;
        result_ = end;
    }
}

}  // namespace ugh::game
