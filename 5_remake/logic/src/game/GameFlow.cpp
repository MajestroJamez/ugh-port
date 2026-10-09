#include "game/GameFlow.hpp"

#include "game/phases/BlackScreen.hpp"
#include "game/phases/CaptionFadeIn.hpp"
#include "game/phases/CaptionFadeOut.hpp"
#include "game/phases/CaptionWaitKey.hpp"
#include "game/phases/Playing.hpp"

namespace ugh::game {

/** The order of the phases lives here: each gets the one after it; Playing ends the attempt (endAttempt). */
GameFlow::GameFlow(Attempts attempts) : attempts_(attempts) {
    slot(PhaseId::BlackBeforeCaption) =
        std::make_unique<phases::BlackScreen>(PhaseId::CaptionFadeIn, GamePhase::BetweenLevels);
    slot(PhaseId::CaptionFadeIn) = std::make_unique<phases::CaptionFadeIn>(PhaseId::CaptionWaitKey);
    slot(PhaseId::CaptionWaitKey) = std::make_unique<phases::CaptionWaitKey>(PhaseId::CaptionFadeOut);
    slot(PhaseId::CaptionFadeOut) = std::make_unique<phases::CaptionFadeOut>(PhaseId::BlackBeforePlay);
    slot(PhaseId::BlackBeforePlay) = std::make_unique<phases::BlackScreen>(PhaseId::Playing, GamePhase::Setup);
    slot(PhaseId::Playing) = std::make_unique<phases::Playing>();
}

void GameFlow::restart() {
    current_ = nullptr;
    result_ = GameResult::Continue;
    resuming_ = false;
}

void GameFlow::resume() {
    restart();
    resuming_ = true;
}

GameResult GameFlow::step() {
    if (result_ != GameResult::Continue) return result_;
    if (current_) {
        current_->nextFrame(*this);
    } else if (resuming_) {
        resuming_ = false;
        goTo(PhaseId::CaptionFadeIn);
    } else {
        attempts_.startGame();
        goTo(PhaseId::BlackBeforeCaption);
    }
    return result_;
}

void GameFlow::goTo(PhaseId next) {
    current_ = slot(next).get();
    current_->enter(*this);
}

void GameFlow::endAttempt() {
    GameResult end = attempts_.end();
    if (end == GameResult::Continue) {
        goTo(PhaseId::CaptionFadeIn);
    } else {
        current_ = nullptr;
        result_ = end;
    }
}

}  // namespace ugh::game
