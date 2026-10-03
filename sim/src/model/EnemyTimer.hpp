// A word of the original an enemy uses for three things.
#pragma once

#include "core/Fixed.hpp"
#include "core/Word.hpp"
#include "model/SharedWord.hpp"

namespace ugh::model {

/**
 * The enemy's timer word: a countdown in most states, the speed of a hit flyer falling out of the sky, and how much
 * faster a charging walker runs.
 */
class EnemyTimer : public SharedWord {
public:
    EnemyTimer() = default;
    explicit EnemyTimer(core::Word word) : SharedWord(word) {}

    void startCountdown(core::Word frames) { word_ = frames; }
    /** One frame of the countdown: true when it just reached zero. */
    bool tick() { return --word_ == 0; }

    /** A hit flyer starts to fall, or a walker to charge, from standstill. */
    void startFromStandstill() { word_ = 0; }

    /** 113b:255e - the falling flyer gets faster, up to 0x28 (1/32 px per frame). */
    void fallFaster() {
        if (word_ < MAX_FALL_SPEED) ++word_;
    }

    /** 113b:2748 - the charging walker gets faster by one in the direction it faces (1/32 px per frame). */
    void speedUpBy(core::Word step) { word_ = step + word_; }

    /** The fall or the extra charge speed, per frame. */
    core::Fixed speed() const { return core::Fixed(word_); }


private:
    static constexpr core::Word MAX_FALL_SPEED = 0x28;
};

}  // namespace ugh::model
