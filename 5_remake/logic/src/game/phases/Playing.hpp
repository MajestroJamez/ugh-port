// The play of a level attempt.
#pragma once

#include "game/Phase.hpp"

namespace ugh::game::phases {

/** The play: a frame of the level after each wait, until the fade-out at the end of the attempt is over. */
class Playing : public Phase {
public:
    void enter(GameFlow& flow) override;
    void nextFrame(GameFlow& flow) override;
    GamePhase reported() const override { return GamePhase::Play; }
};

}  // namespace ugh::game::phases
