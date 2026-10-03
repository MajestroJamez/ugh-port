// A bonus item.
#pragma once

#include "data/kinds/BonusKind.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"
#include "units/Fixed.hpp"
#include "world/Figure.hpp"

namespace ugh::world {
struct PlayContext;
}

namespace ugh::bonuses {

/**
 * A bonus item: dropped by a tree or for a quick delivery, it falls onto a pad and lies there until a copter
 * collects it or its time is up. It shows the sprite of its kind (`world::Figure`, its index is its slot).
 */
class BonusItem : public world::Figure, public state::StateMachine<BonusItem, world::PlayContext> {
public:
    /**
     * A new item of `kind` in `slot` with its middle and bottom at x, y, thrown sideways with `speedX` and up with its
     * kind's lift and `lift` more (Fixed per frame).
     */
    BonusItem(int slot, const data::kinds::BonusKind& kind, units::Fixed x, units::Fixed y, units::Fixed speedX,
              units::Fixed lift);

    const data::kinds::BonusKind& kind() const { return *kind_; }
    /** Fixed per frame. */
    units::Fixed speedX() const { return speedX_; }
    /** Fixed per frame; negative: up. */
    units::Fixed fallSpeed() const { return fallSpeed_; }
    /** Frames it still lies on its pad. */
    int lyingTime() const { return lyingTime_.remaining(); }
    /** It left the play: its slot is free after this frame. */
    bool gone() const { return gone_; }

    // ------------------------------------------------------------ for the states

    /** One frame in its state. */
    void update(const world::PlayContext& context) { updateState(context); }
    /** Its fall (Falling): the speed down after a frame. */
    void setFallSpeed(units::Fixed fallSpeed) { fallSpeed_ = fallSpeed; }
    /** It lies on a pad until its time is up. */
    void startLying(int frames) { lyingTime_.start(frames); }
    bool lyingOver() { return lyingTime_.tick(); }
    void disappear() { gone_ = true; }

private:
    const data::kinds::BonusKind* kind_;
    units::Fixed speedX_;
    units::Fixed fallSpeed_;
    units::Countdown lyingTime_;
    bool gone_ = false;
};

}  // namespace ugh::bonuses
