// The flow of a game, frame by frame.
#pragma once

#include <array>
#include <memory>

#include "game/Attempts.hpp"
#include "game/GamePhase.hpp"
#include "game/GameResult.hpp"
#include "game/Phase.hpp"
#include "game/PhaseId.hpp"

namespace ugh::game {

/**
 * The flow of a game as a state machine of phases; step() is one frame. The phases in their order:
 *
 *   start -> BlackBeforeCaption (8 frames) -> CaptionFadeIn (the attempt starts: the level is loaded; 65 frames)
 *   -> CaptionWaitKey (until a key) -> CaptionFadeOut (65 frames) -> BlackBeforePlay (8 frames)
 *   -> Playing (until the fade-out at the end of the attempt is over)
 *   -> endAttempt(): CaptionFadeIn of the next attempt, or the end of the game
 */
class GameFlow {
public:
    explicit GameFlow(Attempts attempts);
    GameFlow(const GameFlow&) = delete;
    GameFlow& operator=(const GameFlow&) = delete;

    /** A new game: the next step starts it. */
    void restart();

    /** One frame. */
    GameResult step();

    /** Where the game is. */
    GamePhase phase() const { return current_ ? current_->reported() : GamePhase::Start; }

    // ------------------------------------------------------------ for the phases

    Attempts& attempts() { return attempts_; }

    /** The current phase is over: `next` starts in this frame. */
    void goTo(PhaseId next);

    /** The attempt is over: the next level when it is done, else a life less; another attempt, or the end. */
    void endAttempt();

private:
    Attempts attempts_;
    std::array<std::unique_ptr<Phase>, 6> phases_;   // by PhaseId
    Phase* current_ = nullptr;                       // nullptr: not started, or over
    GameResult result_ = GameResult::Continue;
};

}  // namespace ugh::game
