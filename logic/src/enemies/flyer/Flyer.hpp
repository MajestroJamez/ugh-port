// The flyer.
#pragma once

#include "data/FlyerKind.hpp"
#include "data/FlyerPlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/flyer/FlyerState.hpp"
#include "units/Countdown.hpp"
#include "world/Facing.hpp"

namespace ugh::enemies::flyer {

/**
 * The flyer (pterodactyl): it waits hidden, screeches, and flies across the screen at the height of its target's
 * copter - in the team mode the players take turns - and ends the attempt when it touches that copter. A standing
 * passenger dropped onto it makes it fall off the screen.
 */
class Flyer : public Enemy {
public:
    Flyer(int index, const data::FlyerKind& kind, const data::FlyerPlacement& placement);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::FlyerKind& kind() const { return *kind_; }
    const FlyerState& state() const { return *state_; }

    /** Into `next` from the next frame on. */
    void changeState(const FlyerState& next, const EnemyContext& context);
    /** Into `next` and on in it in this frame. */
    void continueIn(const FlyerState& next, const EnemyContext& context);

    /** Fixed per frame; negative: to the left. */
    units::Fixed speedX() const { return vx_; }
    void headLeft();
    void headRight();

    /** The player it hunted last (before its first flight: 1, so that it hunts player 0 first). */
    int lastTarget() const { return lastTarget_; }
    /** The next target: the other player in the team mode, else player 0. */
    int takeNextTarget(int players);

    /** The side it flies towards (its flight animation). */
    world::Facing flight() const { return flight_; }
    void setFlight(world::Facing flight) { flight_ = flight; }

    units::Int16 startDelay() const { return startDelay_; }
    void startWaitTime() { waitTime_.start(startDelay_); }
    bool waitTimeOver() { return waitTime_.tick(); }
    units::Int16 waitTime() const { return waitTime_.remaining(); }
    void startScreechTime(units::Int16 frames) { screechTime_.start(frames); }
    bool screechTimeOver() { return screechTime_.tick(); }
    units::Int16 screechTime() const { return screechTime_.remaining(); }

    /** Falling: 1/32 px per frame, faster every frame up to a limit. */
    units::Int16 fallSpeed() const { return fallSpeed_; }
    void startFalling() { fallSpeed_ = 0; }
    void fallFaster(units::Int16 limit) {
        if (fallSpeed_ < limit) fallSpeed_ += 1;
    }

private:
    const data::FlyerKind* kind_;
    const FlyerState* state_;
    units::Int16 startDelay_;
    units::Fixed vx_;
    int lastTarget_ = 1;
    world::Facing flight_ = world::Facing::Left;
    units::Countdown waitTime_, screechTime_;
    units::Int16 fallSpeed_;
};

}  // namespace ugh::enemies::flyer
