// A phase that lasts a number of retraces.
#pragma once

#include "core/Countdown.hpp"
#include "game/FlowPhase.hpp"

namespace ugh::game::phases {

/** A phase of a fixed number of retraces (a fade, a black screen), then the next phase. */
class TimedPhase : public FlowPhase {
public:
    TimedPhase(int retraces, FlowPhase& next) : retraces_(retraces), next_(next) {}

    void enter(GameFlow& flow) override;
    void afterRetrace(GameFlow& flow) override;

protected:
    /** The work at the start of the phase, before its first wait. */
    virtual void begin(GameFlow&) {}

private:
    const int retraces_;
    FlowPhase& next_;
    core::Countdown left_;
};

}  // namespace ugh::game::phases
