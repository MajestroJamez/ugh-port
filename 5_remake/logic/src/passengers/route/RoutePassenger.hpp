// A passenger with a route.
#pragma once

#include <optional>

#include "data/PassengerKind.hpp"
#include "data/Route.hpp"
#include "data/RoutePassengerPlacement.hpp"
#include "passengers/Passenger.hpp"
#include "passengers/route/RouteState.hpp"
#include "passengers/route/WaitingSpot.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"
#include "units/Int16.hpp"
#include "units/Speed.hpp"
#include "world/Copter.hpp"
#include "world/Pad.hpp"

namespace ugh::passengers::route {

/**
 * A passenger that rides its route from pad to pad. It decides from where it was seen in the last frame (seenX,
 * seenY): the position is taken at the end of a frame while it is shown, so a hidden passenger keeps it. A passenger
 * that falls into the water stays the same passenger: only its kind turns into the water kind, and back when a copter
 * rescues it.
 */
class RoutePassenger : public Passenger, public state::StateMachine<RoutePassenger, PassengerContext> {
public:
    /** From the copter's left edge to where a passenger gets in and out, in pixels. */
    static constexpr units::Int16 COPTER_DOOR = 16;

    RoutePassenger(int index, const data::RoutePassengerPlacement& placement);

    void update(const PassengerContext& context) override;
    void frameShown() override;
    void accept(PassengerVisitor& visitor) const override;

    const data::PassengerKind& kind() const { return *kind_; }

    // ------------------------------------------------------------ the state machine

    /** Into the water and out of it: the kind turns into its counterpart. */
    void switchKind() { kind_ = kind_->other; }

    // ------------------------------------------------------------ the route

    /** The stop of the route it is on (the stop count when the route is done). */
    int routeStop() const { return stop_; }
    bool routeFinished() const { return stop_ >= route_->stopCount(); }
    void nextStop() { stop_++; }
    int pickupPad() const { return route_->stop(stop_).pickupPad; }
    int targetPad() const { return route_->stop(stop_).targetPad; }

    /** The stop starts: the passenger comes out after the delay of the stop. */
    void startArrivalDelay() { arrivalDelay_ = route_->stop(stop_).delay; }
    /** One frame of the delay before it comes out; true when it is over (a delay of 0 is none). */
    bool arrivalDue();
    units::Int16 arrivalDelay() const { return arrivalDelay_; }

    // ------------------------------------------------------------ where it is

    units::Int16 seenX() const { return seenX_; }
    units::Int16 seenY() const { return seenY_; }
    /** The x of its feet (the middle of its sprite) as seen in the last frame. */
    units::Int16 feetX() const { return seenX_ + kind_->box.x; }

    void moveToX(units::Fixed x) { x_ = x; }
    void moveToY(units::Fixed y) { y_ = y; }
    /** Out of the door: its feet at the door, on the pad, and seen there. */
    void standAtDoor(const data::PadDefinition& pad);
    /** One pixel to the left (-1) or to the right (1). */
    void stepBy(int pixels) { x_ += units::Fixed::fromPixels(pixels); }
    /** A step (or a swim stroke) towards the copter; true when it is at the copter's door. */
    bool walkTowards(const world::Copter& copter);
    /** A swimmer stays on the surface (when it was seen elsewhere). */
    void floatOnSurface(units::Int16 waterRow);

    // ------------------------------------------------------------ what it shows

    void restartAnimation() { animator_.restart(); }
    /** One frame of its kind's animation delay; true when the next frame is due. */
    bool animate() { return animator_.step(kind_->animDelay); }
    void rewindAnimation() { animator_.rewind(); }
    /** Shows the frame of `animation`, from its start again after its end. */
    void show(const data::Animation& animation) { sprite_ = animator_.show(animation); }
    /** Shows the frame of `animation` that runs once. */
    void showFrameOf(const data::Animation& animation) { sprite_ = animator_.frameOf(animation); }
    bool pastEndOf(const data::Animation& animation) const { return animator_.pastEndOf(animation); }
    bool atLastFrameOf(const data::Animation& animation) const { return animator_.atLastFrameOf(animation); }
    void showBubble(int bubble) { bubble_ = bubble; }
    void hideBubble() { bubble_.reset(); }

    // ------------------------------------------------------------ waiting, calling, riding, swimming

    WaitingSpot waitingSpot() const { return waitingSpot_; }
    void setWaitingSpot(WaitingSpot spot) { waitingSpot_ = spot; }

    /** How long it calls a copter or waves at it. */
    void startCallTime(units::Int16 frames) { callTime_.start(frames); }
    bool callTimeOver() { return callTime_.tick(); }
    units::Int16 callTime() const { return callTime_.remaining(); }

    /** The copter that carries it. */
    std::optional<int> carrier() const { return carrier_; }
    void setCarrier(int player) { carrier_ = player; }

    /** A delivery before this runs out drops a bonus item. */
    void startQuickDeliveryTime(units::Int16 frames) { quickDeliveryTime_ = frames; }
    void tickQuickDeliveryTime() {
        if (quickDeliveryTime_ > 0) quickDeliveryTime_ -= 1;
    }
    bool deliveredQuickly() const { return quickDeliveryTime_ != 0; }
    units::Int16 quickDeliveryTime() const { return quickDeliveryTime_; }

    /** The speed of a swimmer going under and up (1/64 Fixed per frame). */
    units::Speed swimSpeed() const { return swimSpeed_; }
    void setSwimSpeed(units::Speed speed) { swimSpeed_ = speed; }

    /** How long it stays afloat. */
    void startSwimTime(units::Int16 frames) { swimTime_.start(frames); }
    bool swimTimeOver() { return swimTime_.tick(); }
    units::Int16 swimTime() const { return swimTime_.remaining(); }

private:
    const data::PassengerKind* kind_;
    const data::Route* route_;
    int stop_ = 0;
    units::Int16 seenX_, seenY_;
    units::Int16 arrivalDelay_;
    WaitingSpot waitingSpot_ = WaitingSpot::Starting;
    units::Countdown callTime_;
    std::optional<int> carrier_;
    units::Int16 quickDeliveryTime_;
    units::Speed swimSpeed_;
    units::Countdown swimTime_;
};

}  // namespace ugh::passengers::route
