// The walker.
#pragma once

#include "data/kinds/Facing.hpp"
#include "data/kinds/WalkerKind.hpp"
#include "data/levels/WalkerPlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/Stun.hpp"
#include "enemies/walker/Charge.hpp"
#include "enemies/walker/WalkerState.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"
#include "world/copter/Copter.hpp"
#include "world/scenery/Pad.hpp"

namespace ugh::enemies::walker {

/**
 * The walker (triceratops): it walks to and fro on its pad. When a copter lands there it watches it a while, then
 * charges at it, faster and faster, and throws it into the air; a passenger dropped onto it stuns it.
 */
class Walker : public Enemy, public state::StateMachine<Walker, EnemyContext, WalkerState> {
public:
    /** The walker of `placement` on `pad` (the pad the placement names). */
    Walker(int index, const data::kinds::WalkerKind& kind, const data::levels::WalkerPlacement& placement,
           const world::scenery::Pad& pad);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::kinds::WalkerKind& kind() const { return *kind_; }

    /** The pad it walks on. */
    const world::scenery::Pad& pad() const { return *pad_; }
    /** Fixed per frame; negative: to the left. */
    units::Fixed speedX() const { return vx_; }
    data::kinds::Facing facing() const { return facing_; }
    /** It reached the end of its pad: back the other way. */
    void turnAround();
    /** It faces the copter and runs towards it. */
    void turnTo(const world::copter::Copter& copter);
    /** Shows the frame of the variant of `pair` it faces. */
    void showFacing(const data::kinds::AnimationPair& pair) { show(pair.towards(facing_)); }
    /** One frame of its animation delay (FRAME_DELAY; it watches a copter slower: Watching). */
    bool animate() { return Figure::animate(FRAME_DELAY); }
    using Figure::animate;

    /** It watches a landed copter for `frames` before it charges. */
    void startWatching(int frames) { watchTime_.start(frames); }
    /** One frame of watching; true when it is time to charge. */
    bool tickWatch() { return watchTime_.tick(); }
    int watchTime() const { return watchTime_.remaining(); }

    /** How much faster than its walk it charges. */
    Charge& charge() { return charge_; }
    const Charge& charge() const { return charge_; }
    /** How fast it runs while it charges: its walk and the charge (Fixed per frame). */
    units::Fixed runSpeed() const { return vx_ + charge_.speed(); }

    /** How long it stays stunned. */
    Stun& stun() { return stun_; }
    const Stun& stun() const { return stun_; }

private:
    static constexpr int FRAME_DELAY = 4;   // frames per animation frame

    const data::kinds::WalkerKind* kind_;
    const world::scenery::Pad* pad_;
    units::Fixed vx_;
    // it starts facing left whatever its speed, as in the original: every walker of the data starts walking left
    data::kinds::Facing facing_ = data::kinds::Facing::Left;
    units::Countdown watchTime_;
    Stun stun_;
    Charge charge_;
};

}  // namespace ugh::enemies::walker
