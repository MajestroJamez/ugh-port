// Where an entity is in its animation.
#pragma once

#include "data/Animation.hpp"
#include "units/Countdown.hpp"
#include "units/Int16.hpp"

namespace ugh::world {

/**
 * The frame of an animation an entity shows and the countdown to the next one. The animation itself is not kept: a
 * state shows the one that fits (walking left or right, waving ...) at the current frame, so the frame stays when the
 * state switches to another animation.
 */
class Animator {
public:
    /** From the start: the next step shows frame 0. */
    void restart() {
        frame_ = -1;
        delay_.start(1);
    }

    /** Back to frame 0 without waiting. */
    void rewind() { frame_ = 0; }

    /** One frame of the delay; true when it ran out: the next frame is due and the delay starts again at `delay`. */
    bool step(units::Int16 delay) {
        if (!delay_.tick()) return false;
        delay_.start(delay);
        frame_++;
        return true;
    }

    /** The sprite of the frame in `animation`, from its start again after its end. */
    int show(const data::Animation& animation) {
        if (animation.endsAt(frame_)) frame_ = 0;
        return animation.frame(frame_);
    }

    /** The sprite of the frame in `animation` (an animation that runs once). */
    int frameOf(const data::Animation& animation) const { return animation.frame(frame_); }

    /** The animation that runs once is over. */
    bool pastEndOf(const data::Animation& animation) const { return animation.endsAt(frame_); }
    /** The frame is the last one of `animation`. */
    bool atLastFrameOf(const data::Animation& animation) const { return animation.endsAt(frame_ + 1); }

    int frame() const { return frame_; }
    units::Int16 delay() const { return delay_.remaining(); }

private:
    int frame_ = 0;
    units::Countdown delay_;
};

}  // namespace ugh::world
