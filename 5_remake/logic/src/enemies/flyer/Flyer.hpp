// The flyer.
#pragma once

#include "data/kinds/FlyerKind.hpp"
#include "data/levels/FlyerPlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/flyer/FlyerState.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"
#include "world/Facing.hpp"

namespace ugh::enemies::flyer {

/**
 * The flyer (pterodactyl): it waits hidden, screeches, and flies across the screen at the height of its target's
 * copter - in the team mode the players take turns - and ends the attempt when it touches that copter. A standing
 * passenger dropped onto it makes it fall off the screen.
 */
class Flyer : public Enemy, public state::StateMachine<Flyer, EnemyContext> {
public:
    Flyer(int index, const data::kinds::FlyerKind& kind, const data::levels::FlyerPlacement& placement);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::kinds::FlyerKind& kind() const { return *kind_; }

    /** Fixed per frame; negative: to the left. */
    units::Fixed speedX() const { return vx_; }
    /** It flies towards `side`: its speed and its flight animation point there. */
    void flyTowards(world::Facing side);

    /** The player it hunted last (before its first flight: 1, so that it hunts player 0 first). */
    int lastTarget() const { return lastTarget_; }
    /** The next target: the other player in the team mode, else player 0. */
    int takeNextTarget(int players);

    /** The side it flies towards (its flight animation). */
    world::Facing flight() const { return flight_; }

    int startDelay() const { return startDelay_; }
    /** It stays in its state (hidden, screeching) for `frames` before the next one. */
    void wait(int frames) { waitTime_.start(frames); }
    /** One frame of the wait; true when it is over. */
    bool waitOver() { return waitTime_.tick(); }
    int waitTime() const { return waitTime_.remaining(); }

    /** Falling: 1/32 px per frame, faster every frame up to a limit. */
    int fallSpeed() const { return fallSpeed_; }
    void startFalling() { fallSpeed_ = 0; }
    void fallFaster(int limit) {
        if (fallSpeed_ < limit) fallSpeed_ += 1;
    }

private:
    const data::kinds::FlyerKind* kind_;
    int startDelay_ = 0;
    units::Fixed vx_;
    int lastTarget_ = 1;
    world::Facing flight_ = world::Facing::Left;
    units::Countdown waitTime_;
    int fallSpeed_ = 0;
};

}  // namespace ugh::enemies::flyer
