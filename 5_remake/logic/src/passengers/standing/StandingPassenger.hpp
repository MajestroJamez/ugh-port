// The standing passenger.
#pragma once

#include "data/kinds/StandingPassengerKind.hpp"
#include "data/levels/StandingPassengerPlacement.hpp"
#include "passengers/Passenger.hpp"
#include "passengers/standing/StandingState.hpp"
#include "state/StateMachine.hpp"
#include "units/Fixed.hpp"
#include "world/copter/Copter.hpp"

namespace ugh::passengers::standing {

/**
 * The standing passenger: it waits on its pad until a copter with room touches it, hangs below the copter, and falls
 * where the pilot lets it go - onto a pad to stand there again, onto an enemy to stun it, or off the screen.
 */
class StandingPassenger : public Passenger,
                          public state::StateMachine<StandingPassenger, PassengerContext, StandingState> {
public:
    StandingPassenger(int index, const data::levels::StandingPassengerPlacement& placement);

    void update(const PassengerContext& context) override;
    void accept(PassengerVisitor& visitor) const override;

    const data::kinds::StandingPassengerKind& kind() const { return *kind_; }

    /** The copter it hangs below (or last hung below); nullptr before. */
    world::copter::Copter* carrier() { return carrier_; }
    const world::copter::Copter* carrier() const { return carrier_; }
    /** A copter with room picked it up. */
    void hangBelow(world::copter::Copter& copter) { carrier_ = &copter; }

    // ------------------------------------------------------------ falling

    /** Its speed sideways and down (Fixed per frame; negative: up). */
    units::Fixed dropSpeedX() const { return dropSpeedX_; }
    units::Fixed fallSpeed() const { return fallSpeed_; }
    void setFall(units::Fixed speedX, units::Fixed fallSpeed) {
        dropSpeedX_ = speedX;
        fallSpeed_ = fallSpeed;
    }
    /** It bounced off an enemy: it goes up with `fallSpeed`, showing that it was hit. */
    void bounce(units::Fixed fallSpeed, int bouncedSprite) {
        fallSpeed_ = fallSpeed;
        showSprite(bouncedSprite);
    }
    /** It bounced off a blower: the same, but it shows no hit. */
    void bounceUnseen(units::Fixed fallSpeed) { fallSpeed_ = fallSpeed; }

    /** It falls down (not up) and its hit point is in the box of an enemy at enemyX, enemyY. */
    bool fallsOnto(units::Fixed enemyX, units::Fixed enemyY) const;

private:
    const data::kinds::StandingPassengerKind* kind_;
    world::copter::Copter* carrier_ = nullptr;
    units::Fixed dropSpeedX_;
    units::Fixed fallSpeed_;
};

}  // namespace ugh::passengers::standing
