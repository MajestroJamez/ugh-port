#include "game/GameFlow.hpp"

#include "game/Game.hpp"
#include "ugh_sim.h"

namespace ugh::game {

GameFlow::GameFlow(Game& game)
    : game_(game),
      blackBeforePlay_(playing_),
      captionFadeOut_(captionFadeIn_, blackBeforePlay_),   // CaptionFadeIn is built later: only its reference is kept
      captionWaitKey_(captionFadeOut_),
      captionFadeIn_(captionWaitKey_),
      blackBeforeCaption_(captionFadeIn_),
      result_(UGH_SIM_CONTINUE) {}

void GameFlow::reset() {
    phase_ = nullptr;
    result_ = UGH_SIM_CONTINUE;
}

/** 113b:0c61 - GameFlow.kt playGame: a new game, then black, then the level attempts. */
int GameFlow::step() {
    if (result_ != UGH_SIM_CONTINUE) return result_;
    if (phase_) {
        phase_->afterRetrace(*this);
    } else {
        game_.newGame();
        moveTo(blackBeforeCaption_);
    }
    return result_;
}

void GameFlow::moveTo(FlowPhase& next) {
    phase_ = &next;
    next.enter(*this);
}

void GameFlow::endAttempt() {
    int end = game_.levelEnd();
    if (end == UGH_SIM_CONTINUE) {
        moveTo(captionFadeIn_);
    } else {
        phase_ = nullptr;
        result_ = end;
    }
}

}  // namespace ugh::game
