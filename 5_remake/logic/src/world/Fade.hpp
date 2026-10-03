// The fade of a level attempt.
#pragma once

namespace ugh::world {

/**
 * The fade of a level attempt: it fades in at the start and fades out at the end; the attempt is over when the
 * fade-out reaches black. The play goes on during both, but the copters wait until the fade-in is three quarters done.
 */
class Fade {
public:
    static constexpr int FULL = 256;
    static constexpr int COPTERS_START = 192;
    static constexpr int STEP = 2;

    /** From black, up. */
    void startFadeIn() {
        position_ = 0;
        out_ = false;
    }

    /** The fade-out that ends the attempt starts. */
    void startFadeOut() {
        out_ = true;
        position_ -= STEP;
    }

    /** One frame of the fade (it stops at the top). */
    void advance() {
        if (position_ <= FULL) position_ += out_ ? -STEP : STEP;
    }

    bool fadingOut() const { return out_; }
    /** The fade-out reached black: the attempt is over. */
    bool over() const { return position_ < 0; }
    /** The copters stand still while the level fades in, until three quarters of the way. */
    bool coptersWaiting() const { return position_ <= COPTERS_START && !out_; }
    int position() const { return position_; }

private:
    int position_ = 0;
    bool out_ = false;
};

}  // namespace ugh::world
