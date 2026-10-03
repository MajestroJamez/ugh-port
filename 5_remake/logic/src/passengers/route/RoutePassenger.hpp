// A passenger with a route.
#pragma once

#include "data/PadDefinition.hpp"
#include "data/PassengerKind.hpp"
#include "data/RoutePassengerPlacement.hpp"
#include "passengers/Passenger.hpp"
#include "passengers/route/PassengerCall.hpp"
#include "passengers/route/Ride.hpp"
#include "passengers/route/Route.hpp"
#include "passengers/route/RouteState.hpp"
#include "passengers/route/Swim.hpp"
#include "state/StateMachine.hpp"
#include "units/Int16.hpp"
#include "world/Copter.hpp"

namespace ugh::passengers::route {

/**
 * A passenger that rides its route from pad to pad. Its parts keep what belongs together: `route()` the stop it is on,
 * `call()` waiting and calling a copter, `ride()` the copter it rides in, `swim()` the water. It decides from where it
 * was seen in the last frame (seenX, seenY): the position is taken at the end of a frame while it is shown, so a
 * hidden passenger keeps it. A passenger that falls into the water stays the same passenger: only its kind turns into
 * the water kind, and back when a copter rescues it.
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
    /** Into the water and out of it: the kind turns into its counterpart. */
    void switchKind() { kind_ = kind_->other; }

    Route& route() { return route_; }
    const Route& route() const { return route_; }
    PassengerCall& call() { return call_; }
    const PassengerCall& call() const { return call_; }
    Ride& ride() { return ride_; }
    const Ride& ride() const { return ride_; }
    Swim& swim() { return swim_; }
    const Swim& swim() const { return swim_; }

    // ------------------------------------------------------------ where it is

    units::Int16 seenX() const { return seenX_; }
    units::Int16 seenY() const { return seenY_; }
    /** The x of its feet (the middle of its sprite) as seen in the last frame. */
    units::Int16 feetX() const { return seenX_ + kind_->box.x; }

    /** Out of the door: its feet at the door, on the pad, and seen there. */
    void standAtDoor(const data::PadDefinition& pad);
    /** One pixel to the left (-1) or to the right (1). */
    void stepBy(int pixels) { moveToX(x() + units::Fixed::fromPixels(pixels)); }
    /** A step (or a swim stroke) towards the copter; true when it is at the copter's door. */
    bool walkTowards(const world::Copter& copter);
    /** A swimmer stays on the surface (when it was seen elsewhere). */
    void floatOnSurface(units::Int16 waterRow);

    /** One frame of its kind's animation delay; true when the next frame is due. */
    bool animate() { return Figure::animate(kind_->animDelay); }

private:
    const data::PassengerKind* kind_;
    Route route_;
    PassengerCall call_;
    Ride ride_;
    Swim swim_;
    units::Int16 seenX_, seenY_;
};

}  // namespace ugh::passengers::route
