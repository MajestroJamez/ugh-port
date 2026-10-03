// The standing passenger.
#pragma once

#include <optional>

#include "data/PassengerKind.hpp"
#include "data/StandingPassengerPlacement.hpp"
#include "passengers/Passenger.hpp"
#include "passengers/standing/StandingState.hpp"
#include "state/StateMachine.hpp"
#include "units/Fixed.hpp"
#include "units/Int16.hpp"

namespace ugh::passengers::standing {

/**
 * The standing passenger: it waits on its pad until a copter with room touches it, hangs below the copter, and falls
 * where the pilot lets it go - onto a pad to stand there again, onto an enemy to stun it, or off the screen.
 */
class StandingPassenger : public Passenger, public state::StateMachine<StandingPassenger, PassengerContext, StandingState> {
public:
    StandingPassenger(int index, const data::StandingPassengerPlacement& placement);

    void update(const PassengerContext& context) override;
    void accept(PassengerVisitor& visitor) const override;

    const data::PassengerKind& kind() const { return *kind_; }


    /** The copter it hangs below. */
    std::optional<int> carrier() const { return carrier_; }
    void setCarrier(int player) { carrier_ = player; }

    void moveTo(units::Fixed x, units::Fixed y) {
        x_ = x;
        y_ = y;
    }
    void showSprite(int sprite) { sprite_ = sprite; }

    // ------------------------------------------------------------ falling

    /** Its speed sideways (Fixed per frame) and down (1/32 px per frame; negative: up). */
    units::Fixed dropSpeedX() const { return dropSpeedX_; }
    units::Int16 fallSpeed() const { return fallSpeed_; }
    void setFall(units::Fixed speedX, units::Int16 fallSpeed) {
        dropSpeedX_ = speedX;
        fallSpeed_ = fallSpeed;
    }
    /** It bounced off an enemy: it goes up with `fallSpeed`, showing that it was hit. */
    void bounce(units::Int16 fallSpeed, int bouncedSprite) {
        fallSpeed_ = fallSpeed;
        sprite_ = bouncedSprite;
    }
    /** It bounced off a blower: the same, but it shows no hit. */
    void bounceUnseen(units::Int16 fallSpeed) { fallSpeed_ = fallSpeed; }

    /** It falls down (not up) and its hit point is in the box of an enemy at x, y. */
    bool fallsOnto(units::Fixed x, units::Fixed y) const;

private:
    const data::PassengerKind* kind_;
    std::optional<int> carrier_;
    units::Fixed dropSpeedX_;
    units::Int16 fallSpeed_;
};

}  // namespace ugh::passengers::standing
