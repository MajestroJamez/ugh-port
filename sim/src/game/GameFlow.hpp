// The flow of a game: from a new game to its end, frame by frame.
#pragma once

#include "game/FlowPhase.hpp"
#include "game/phases/BlackScreen.hpp"
#include "game/phases/CaptionFadeIn.hpp"
#include "game/phases/CaptionFadeOut.hpp"
#include "game/phases/CaptionWaitKey.hpp"
#include "game/phases/Playing.hpp"

namespace ugh::game {

class Game;

/**
 * 113b:0c61 .. 0fe7 - GameFlow.kt playGame: the flow of a game as a state machine of phases. step() is one frame of
 * the original: the work from one wait for the vertical retrace to the next. The phases in their order:
 *
 *   new game -> BlackScreen (8 retraces)
 *   -> CaptionFadeIn (a new attempt at the level, its caption; 65 retraces)
 *   -> CaptionWaitKey (until a key) -> CaptionFadeOut (65 retraces)
 *   -> BlackScreen (8 retraces) -> Playing (a frame per retrace until the fade-out is over)
 *   -> endAttempt(): CaptionFadeIn of the next attempt, or the end of the game
 */
class GameFlow {
public:
    explicit GameFlow(Game& game);
    GameFlow(const GameFlow&) = delete;
    GameFlow& operator=(const GameFlow&) = delete;

    /** Not started: the next step starts a new game. */
    void reset();

    /** One frame of the original. UGH_SIM_CONTINUE while the game goes on, else how it ended. */
    int step();

    // ------------------------------------------------------------ for the phases

    Game& game() { return game_; }

    /** The current phase is over: the next one starts right away (in this frame). */
    void moveTo(FlowPhase& next);

    /**
     * 113b:0fa7 - GameFlow.kt playGame (after playLevel): the attempt is over; the next level when it is done, else
     * a life less; another attempt right away, or the end of the game.
     */
    void endAttempt();

private:
    Game& game_;
    // in the reverse order of the flow: each phase is built with the next one
    phases::Playing playing_;
    phases::BlackScreen blackBeforePlay_;
    phases::CaptionFadeOut captionFadeOut_;
    phases::CaptionWaitKey captionWaitKey_;
    phases::CaptionFadeIn captionFadeIn_;
    phases::BlackScreen blackBeforeCaption_;

    FlowPhase* phase_ = nullptr;   // nullptr: the game has not started, or it is over
    int result_;
};

}  // namespace ugh::game
