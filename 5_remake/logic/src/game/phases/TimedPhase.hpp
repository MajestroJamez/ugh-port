// A phase of a number of frames.
#pragma once

#include "game/Phase.hpp"
#include "game/PhaseId.hpp"
#include "units/Countdown.hpp"

namespace ugh::game::phases {

/** A phase of a fixed number of frames (a fade, a black screen), then the next phase. */
class TimedPhase : public Phase {
public:
    TimedPhase(int frames, PhaseId next) : frames_(frames), next_(next) {}

    void enter(GameFlow& flow) override;
    void nextFrame(GameFlow& flow) override;

protected:
    /** The work at the start of the phase. */
    virtual void begin(GameFlow&) {}

private:
    int frames_;
    PhaseId next_;
    units::Countdown left_;
};

}  // namespace ugh::game::phases
