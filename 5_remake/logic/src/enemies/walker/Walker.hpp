// The walker.
#pragma once

#include "data/WalkerKind.hpp"
#include "data/WalkerPlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/walker/WalkerState.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"
#include "world/Copter.hpp"
#include "world/Facing.hpp"

namespace ugh::enemies::walker {

/**
 * The walker (triceratops): it walks to and fro on its pad. When a copter lands there it watches it a while, then
 * charges at it, faster and faster, and throws it into the air; a passenger dropped onto it stuns it.
 */
class Walker : public Enemy, public state::StateMachine<Walker, EnemyContext, WalkerState> {
public:
    Walker(int index, const data::WalkerKind& kind, const data::WalkerPlacement& placement);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::WalkerKind& kind() const { return *kind_; }


    int pad() const { return pad_; }
    /** Fixed per frame; negative: to the left. */
    units::Fixed speedX() const { return vx_; }
    world::Facing facing() const { return facing_; }
    /** It reached the end of its pad: back the other way. */
    void turnAround();
    /** It faces the copter and runs towards it. */
    void turnTo(const world::Copter& copter);
    /** Shows the frame of the variant of `pair` it faces. */
    void showFacing(const data::AnimationPair& pair) { show(pair.towards(facing_ == world::Facing::Right)); }

    void startWatchTime(units::Int16 frames) { watchTime_.start(frames); }
    bool watchTimeOver() { return watchTime_.tick(); }
    units::Int16 watchTime() const { return watchTime_.remaining(); }

    /** How much faster than its walk it charges (Fixed per frame): one more every frame, in the direction it faces. */
    units::Int16 chargeSpeed() const { return chargeSpeed_; }
    void startCharge() { chargeSpeed_ = 0; }
    void chargeFaster() { chargeSpeed_ += facing_ == world::Facing::Right ? 1 : -1; }

    void startStunTime(units::Int16 frames) { stunTime_.start(frames); }
    bool stunTimeOver() { return stunTime_.tick(); }
    units::Int16 stunTime() const { return stunTime_.remaining(); }

private:
    const data::WalkerKind* kind_;
    int pad_;
    units::Fixed vx_;
    world::Facing facing_ = world::Facing::Left;
    units::Countdown watchTime_, stunTime_;
    units::Int16 chargeSpeed_;
};

}  // namespace ugh::enemies::walker
