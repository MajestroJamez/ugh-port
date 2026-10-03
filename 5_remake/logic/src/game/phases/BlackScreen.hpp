// The screen turns black.
#pragma once

#include "game/phases/TimedPhase.hpp"

namespace ugh::game::phases {

/** The palette turns black, 32 colours per frame, before a screen is drawn. */
class BlackScreen : public TimedPhase {
public:
    static constexpr int FRAMES = 8;

    BlackScreen(PhaseId next, GamePhase reported) : TimedPhase(FRAMES, next), reported_(reported) {}

    GamePhase reported() const override { return reported_; }

private:
    GamePhase reported_;
};

}  // namespace ugh::game::phases
