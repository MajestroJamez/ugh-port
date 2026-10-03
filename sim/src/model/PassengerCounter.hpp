// A word of the original a passenger uses for three things.
#pragma once

#include "core/Word.hpp"
#include "model/SharedWord.hpp"

namespace ugh::model {

/**
 * The passenger's word 2a2d, which the original uses for whatever the state needs:
 * - a countdown while the passenger calls a copter or waves impatiently,
 * - whether a waiting passenger stands at its waiting spot (1) or still walks there (0, -1 when it just started),
 * - the copter that carries it (player * 2) while it rides or hangs.
 */
class PassengerCounter : public SharedWord {
public:
    PassengerCounter() = default;
    explicit PassengerCounter(core::Word word) : SharedWord(word) {}

    void startCountdown(core::Word frames) { word_ = frames; }
    /** One frame of the countdown: true when it just reached zero. */
    bool tick() { return --word_ == 0; }

    /** The passenger starts to wait: it walks to its waiting spot first. */
    void startWaiting() { word_ = -1; }
    void walkingToSpot() { word_ = 0; }
    void atSpot() { word_ = 1; }
    bool isAtSpot() const { return word_ == 1; }

    void setCarrier(int player) { word_ = player * 2; }
    int carrier() const { return word_.value() / 2; }
};

}  // namespace ugh::model
