// A word of the original a bonus item uses for two things.
#pragma once

#include "core/Fixed.hpp"
#include "core/Word.hpp"

namespace ugh::model {

/** The bonus item's word 2e5b: its x speed while it falls (1/32 px per frame), then the frames it stays. */
class BonusTimer {
public:
    BonusTimer() = default;
    explicit BonusTimer(core::Word word) : word_(word) {}

    void setSpeedX(core::Fixed speed) { word_ = speed.raw(); }
    core::Fixed speedX() const { return core::Fixed(word_); }

    void startLying(core::Word frames) { word_ = frames; }
    /** One frame on the pad: true when its time is up. */
    bool lyingOver() { return --word_ == 0; }

    core::Word word() const { return word_; }

private:
    core::Word word_;
};

}  // namespace ugh::model
