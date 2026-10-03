// A passenger with a route.
#pragma once

#include "data/PadDefinition.hpp"
#include "data/AnimatedPassengerKind.hpp"
#include "data/RoutePassengerKind.hpp"
#include "data/RoutePassengerPlacement.hpp"
#include "data/SwimmerKind.hpp"
#include "passengers/Passenger.hpp"
#include "passengers/route/PassengerCall.hpp"
#include "passengers/route/Ride.hpp"
#include "passengers/route/Route.hpp"
#include "passengers/route/RouteState.hpp"
#include "passengers/route/Swim.hpp"
#include "state/StateMachine.hpp"
#include "world/Copter.hpp"

namespace ugh::passengers::route {

/**
 * A passenger that rides its route from pad to pad. Its parts keep what belongs together: `route()` the stop it is on,
 * `call()` waiting and calling a copter, `ride()` the copter it rides in, `swim()` the water. It decides from where it
 * was seen in the last frame (seenX, seenY): the position is taken at the end of a frame while it is shown, so a
 * hidden passenger keeps it. A passenger that falls into the water stays the same passenger: its kind is the swimmer kind of
 * its land kind (`swimmerKind()`) until a copter rescues it.
 */
class RoutePassenger : public Passenger, public state::StateMachine<RoutePassenger, PassengerContext> {
public:
    /** From the copter's left edge to where a passenger gets in and out, in pixels. */
    static constexpr int COPTER_DOOR = 16;

    RoutePassenger(int index, const data::RoutePassengerPlacement& placement);

    void update(const PassengerContext& context) override;
    void frameShown() override;
    void accept(PassengerVisitor& visitor) const override;

    /** Its kind as it is now: on land or in the water. */
    const data::AnimatedPassengerKind& kind() const;
    const data::RoutePassengerKind& landKind() const { return *land_; }
    const data::SwimmerKind& swimmerKind() const { return *land_->swimmer; }
    bool inWater() const { return inWater_; }
    /** It falls into the water and is a swimmer, until a copter rescues it. */
    void intoWater() { inWater_ = true; }
    void outOfWater() { inWater_ = false; }

    Route& route() { return route_; }
    const Route& route() const { return route_; }
    PassengerCall& call() { return call_; }
    const PassengerCall& call() const { return call_; }
    Ride& ride() { return ride_; }
    const Ride& ride() const { return ride_; }
    Swim& swim() { return swim_; }
    const Swim& swim() const { return swim_; }

    // ------------------------------------------------------------ where it is

    int seenX() const { return seenX_; }
    int seenY() const { return seenY_; }
    /** The x of its feet (the middle of its sprite) as seen in the last frame. */
    int feetX() const { return seenX_ + kind().box.x; }

    /** Out of the door: its feet at the door, on the pad, and seen there. */
    void standAtDoor(const data::PadDefinition& pad);
    /**
     * A step towards `feet` (the x of its feet): shown walking that way, one pixel further when `stepDue`; true when
     * its feet (as seen in the last frame) are there.
     */
    bool stepTowards(int feet, bool stepDue = true);
    /** A step (or a swim stroke) towards the copter; true when it is at the copter's door. */
    bool walkTowards(const world::Copter& copter);
    /** A swimmer stays on the surface (when it was seen elsewhere). */
    void floatOnSurface(int waterRow);

    /** One frame of its kind's animation delay; true when the next frame is due. */
    bool animate() { return Figure::animate(kind().animDelay); }

private:
    const data::RoutePassengerKind* land_;
    bool inWater_ = false;
    Route route_;
    PassengerCall call_;
    Ride ride_;
    Swim swim_;
    int seenX_ = 0, seenY_ = 0;
};

}  // namespace ugh::passengers::route
