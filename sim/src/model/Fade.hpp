// The palette fade of a level attempt.
#pragma once

#include "core/Word.hpp"

namespace ugh::model {

/**
 * The palette fade (27a8, 27aa): the level fades in at its start and fades out at its end; the attempt is over when
 * the fade-out reaches black. The play runs during both.
 */
class Fade {
public:
    static constexpr core::Word FULL = 0x100;            // the level fully visible
    static constexpr core::Word COPTERS_START = 0xc0;    // the copters fly from here on while it fades in

    Fade() = default;
    Fade(core::Word position, core::Word step) : position_(position), step_(step) {}

    /** The start of an attempt: from black, up. */
    void startFadeIn() {
        position_ = 0;
        step_ = 2;
    }

    /** Starts the fade-out that ends the level attempt. */
    void startFadeOut() {
        step_ = -2;
        position_ -= 2;
    }

    /** 113b:0c7d - one frame of the fade (it stops at the top). */
    void advance() {
        if (position_ <= FULL) position_ += step_;
    }

    bool fadingOut() const { return step_ < 0; }

    /** The fade-out reached black: the attempt is over. */
    bool over() const { return position_ < 0; }

    /** The copters stand still while the level fades in, until three quarters of the way. */
    bool coptersWaiting() const { return position_ <= COPTERS_START && step_ > 0; }

    core::Word position() const { return position_; }
    core::Word step() const { return step_; }

private:
    core::Word position_;   // 0 black .. FULL
    core::Word step_;       // per frame; negative fades out
};

}  // namespace ugh::model
