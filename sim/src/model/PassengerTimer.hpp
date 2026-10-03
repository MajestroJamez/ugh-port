// Another word of the original a passenger uses for three things.
#pragma once

#include "core/Fixed.hpp"
#include "core/Word.hpp"
#include "model/SharedWord.hpp"

namespace ugh::model {

/**
 * The passenger's word 2acd, which the original uses for whatever the state needs:
 * - the delay before a walking passenger comes out of its door,
 * - the x speed of the standing passenger dropped from a copter (1/32 px per frame),
 * - how long a swimmer stays afloat.
 */
class PassengerTimer : public SharedWord {
public:
    PassengerTimer() = default;
    explicit PassengerTimer(core::Word word) : SharedWord(word) {}

    void startArrivalDelay(core::Word frames) { word_ = frames; wrote("arrival"); }
    /** 113b:1509 - one frame of the delay before appearing: true when it is over (a delay of 0 is none). */
    bool arrivalDue() { reading("PassengerTimer", "arrival"); wrote("arrival"); return word_ == 0 || --word_ == 0; }

    void setDropSpeed(core::Fixed speedX) { word_ = speedX.raw(); wrote("dropSpeed"); }
    core::Fixed dropSpeed() const { reading("PassengerTimer", "dropSpeed"); return core::Fixed(word_); }

    void startSwimTime(core::Word frames) { word_ = frames; wrote("swim"); }
    /** One frame afloat: true when the time is up. */
    bool swimTimeUp() { reading("PassengerTimer", "swim"); wrote("swim"); return --word_ == 0; }
};

}  // namespace ugh::model
