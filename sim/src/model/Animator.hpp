// Where an entity is in its animation.
#pragma once

#include "core/Countdown.hpp"
#include "core/Word.hpp"
#include "data/Animation.hpp"
#include "data/Sprite.hpp"

namespace ugh::model {

/**
 * The frame of an animation an entity shows and the countdown to the next one (passengers and enemies). The
 * animation itself is not kept: the state shows whichever fits (walking left or right, waving ...) at the frame.
 */
class Animator {
public:
    Animator() = default;
    Animator(core::Word frame, core::Word delay) : frame_(frame), delay_(delay) {}

    /** From the start: the next step shows frame 0. */
    void restart() {
        frame_ = -1;
        delay_.start(1);
    }

    /** Back to frame 0 without waiting for the delay. */
    void rewind() { frame_ = 0; }

    /** One frame of the delay; true when it ran out: the next frame is due and the delay starts again at `delay`. */
    bool step(core::Word delay) {
        if (!delay_.tick()) return false;
        delay_.start(delay);
        ++frame_;
        return true;
    }

    /** The sprite of the frame in `animation`, from its start again past the end. */
    data::Sprite show(const data::Animation& animation) {
        while (animation.endsAt(frame_.value())) frame_ = 0;
        return animation.frame(frame_.value());
    }

    /** The sprite of the frame in `animation` as it is (it can be the list end). */
    data::Sprite frameOf(const data::Animation& animation) const { return animation.frame(frame_.value()); }

    /** The frame is past the end of `animation` (it shows the list end). */
    bool pastEndOf(const data::Animation& animation) const { return animation.endsAt(frame_.value()); }

    /** The frame is the last one of `animation`. */
    bool atLastFrameOf(const data::Animation& animation) const { return animation.endsAt(frame_.value() + 1); }

    core::Word frame() const { return frame_; }
    core::Word delay() const { return delay_.remaining(); }

private:
    core::Word frame_;
    core::Countdown delay_;
};

}  // namespace ugh::model
