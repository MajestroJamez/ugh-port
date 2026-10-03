// A bonus item.
#pragma once

#include "data/BonusKind.hpp"
#include "units/Countdown.hpp"
#include "units/Fixed.hpp"
#include "units/Int16.hpp"

namespace ugh::bonuses {

class BonusState;

/**
 * A bonus item: dropped by a tree or for a quick delivery, it falls onto a pad and lies there until a copter
 * collects it or its time is up.
 */
class BonusItem {
public:
    /** A new item of `kind` with its middle and bottom at x, y, thrown sideways with `speedX` and up with `lift`. */
    BonusItem(int slot, const data::BonusKind& kind, units::Fixed x, units::Fixed y, units::Fixed speedX, units::Int16 lift);

    int slot() const { return slot_; }
    const data::BonusKind& kind() const { return *kind_; }
    const BonusState& state() const { return *state_; }
    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    units::Fixed speedX() const { return speedX_; }
    units::Int16 fallSpeed() const { return fallSpeed_; }
    units::Countdown lyingTime() const { return lyingTime_; }
    /** It left the play: its slot is free after this frame. */
    bool gone() const { return gone_; }

    // ------------------------------------------------------------ for the states

    void changeState(const BonusState& next);
    /** Its fall (Falling): position and speed after a frame. */
    void moveTo(units::Fixed x, units::Fixed y, units::Int16 fallSpeed);
    /** It lies on a pad until its time is up. */
    void startLying(units::Int16 frames) { lyingTime_.start(frames); }
    bool lyingOver() { return lyingTime_.tick(); }
    void disappear() { gone_ = true; }

private:
    int slot_;
    const data::BonusKind* kind_;
    const BonusState* state_;
    units::Fixed x_, y_;
    units::Fixed speedX_;
    units::Int16 fallSpeed_;
    units::Countdown lyingTime_;
    bool gone_ = false;
};

}  // namespace ugh::bonuses
