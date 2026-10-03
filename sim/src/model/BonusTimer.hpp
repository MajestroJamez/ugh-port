// A word of the original a bonus item uses for two things.
#pragma once

#include "core/Fixed.hpp"
#include "core/Word.hpp"
#include "model/SharedWord.hpp"

namespace ugh::model {

/** The bonus item's word 2e5b: its x speed while it falls (1/32 px per frame), then the frames it stays. */
class BonusTimer : public SharedWord {
public:
    BonusTimer() = default;
    explicit BonusTimer(core::Word word) : SharedWord(word) {}

    void setSpeedX(core::Fixed speed) { word_ = speed.raw(); wrote("speedX"); }
    core::Fixed speedX() const { reading("BonusTimer", "speedX"); return core::Fixed(word_); }

    void startLying(core::Word frames) { word_ = frames; wrote("lying"); }
    /** One frame on the pad: true when its time is up. */
    bool lyingOver() { reading("BonusTimer", "lying"); wrote("lying"); return --word_ == 0; }
};

}  // namespace ugh::model
