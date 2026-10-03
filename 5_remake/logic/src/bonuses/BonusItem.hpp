// A bonus item.
#pragma once

#include "data/kinds/BonusKind.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"
#include "units/Fixed.hpp"

namespace ugh::world {
struct PlayContext;
}

namespace ugh::bonuses {

/**
 * A bonus item: dropped by a tree or for a quick delivery, it falls onto a pad and lies there until a copter
 * collects it or its time is up.
 */
class BonusItem : public state::StateMachine<BonusItem, world::PlayContext> {
public:
    /** A new item of `kind` with its middle and bottom at x, y, thrown sideways with `speedX` and up with `lift`. */
    BonusItem(int slot, const data::kinds::BonusKind& kind, units::Fixed x, units::Fixed y, units::Fixed speedX, int lift);

    int slot() const { return slot_; }
    const data::kinds::BonusKind& kind() const { return *kind_; }
    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    units::Fixed speedX() const { return speedX_; }
    int fallSpeed() const { return fallSpeed_; }
    units::Countdown lyingTime() const { return lyingTime_; }
    /** It left the play: its slot is free after this frame. */
    bool gone() const { return gone_; }

    // ------------------------------------------------------------ for the states

    /** One frame in its state. */
    void update(const world::PlayContext& context) { updateState(context); }
    /** Its fall (Falling): position and speed after a frame. */
    void moveTo(units::Fixed x, units::Fixed y, int fallSpeed);
    /** It lies on a pad until its time is up. */
    void startLying(int frames) { lyingTime_.start(frames); }
    bool lyingOver() { return lyingTime_.tick(); }
    void disappear() { gone_ = true; }

private:
    int slot_ = 0;
    const data::kinds::BonusKind* kind_;
    units::Fixed x_, y_;
    units::Fixed speedX_;
    int fallSpeed_ = 0;
    units::Countdown lyingTime_;
    bool gone_ = false;
};

}  // namespace ugh::bonuses
