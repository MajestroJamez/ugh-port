// A passenger with a route.
#pragma once

#include "data/SpriteIds.hpp"
#include "data/levels/PadDefinition.hpp"
#include "data/levels/RoutePassengerPlacement.hpp"
#include "passengers/Passenger.hpp"
#include "passengers/route/PassengerForm.hpp"
#include "passengers/route/PickupWait.hpp"
#include "passengers/route/Ride.hpp"
#include "passengers/route/RouteProgress.hpp"
#include "passengers/route/RouteState.hpp"
#include "passengers/route/Swim.hpp"
#include "state/StateMachine.hpp"
#include "world/Level.hpp"
#include "world/copter/Copter.hpp"

namespace ugh::passengers::route {

/**
 * A passenger that rides its route from pad to pad. Its parts keep what belongs together: `route()` the stop it is on,
 * `pickupWait()` waiting for a copter and calling it, `ride()` the copter it rides in, `swim()` the water. It decides
 * from where it was seen in the last frame (seenX, seenY): the position is taken at the end of a frame while it is
 * shown, so a hidden passenger keeps it. A passenger that falls into the water stays the same passenger: its kind is
 * the swimmer kind of its land kind (`form()`) until a copter rescues it.
 */
class RoutePassenger : public Passenger, public state::StateMachine<RoutePassenger, PassengerContext> {
public:
    /** The passenger of `placement`, on its route through the pads of `level`. */
    RoutePassenger(int index, const data::levels::RoutePassengerPlacement& placement, world::Level& level);

    void update(const PassengerContext& context) override;
    void frameShown() override;
    void accept(PassengerVisitor& visitor) const override;

    /** Its kind as it is now: on land or in the water (`form()`). */
    const data::kinds::AnimatedPassengerKind& kind() const { return form_.current(); }
    PassengerForm& form() { return form_; }
    const PassengerForm& form() const { return form_; }

    RouteProgress& route() { return route_; }
    const RouteProgress& route() const { return route_; }
    PickupWait& pickupWait() { return pickupWait_; }
    const PickupWait& pickupWait() const { return pickupWait_; }
    Ride& ride() { return ride_; }
    const Ride& ride() const { return ride_; }
    Swim& swim() { return swim_; }
    const Swim& swim() const { return swim_; }

    // ------------------------------------------------------------ waiting for a copter (on a pad or in the water)

    /** It starts to call a copter: the bubble shows the number of its target pad. */
    void startCalling(const data::SpriteIds& sprites);
    /** It starts to wave impatiently: the impatient bubble. */
    void startWaving(const data::SpriteIds& sprites);
    /** It starts to go to the copter: the bubble goes. */
    void startBoarding();
    /** One frame of calling or waving: the next frame of its waving when it is due. */
    void wave() {
        if (animate()) show(*kind().waving);
    }

    // ------------------------------------------------------------ where it is

    int seenX() const { return seenX_; }
    int seenY() const { return seenY_; }

    /** Out of the door: its feet at the door, on the pad, and seen there. */
    void standAtDoor(const data::levels::PadDefinition& pad);
    /** A step towards `feet` (the x of its feet): shown walking that way, a pixel further; true when it is there. */
    bool stepTowards(int feet);
    /** A step (or a swim stroke) towards the copter when its next frame is due; true when it is at the door. */
    bool walkTowards(const world::copter::Copter& copter);
    /** A swimmer stays on the surface (when it was seen elsewhere). */
    void floatOnSurface(int waterRow);

    /** One frame of its kind's animation delay; true when the next frame is due. */
    bool animate() { return Figure::animate(kind().animDelay); }

private:
    PassengerForm form_;
    RouteProgress route_;
    PickupWait pickupWait_;
    Ride ride_;
    Swim swim_;
    int seenX_ = 0, seenY_ = 0;

    /** The x of its feet (the middle of its sprite) as seen in the last frame. */
    int feetX() const { return seenX_ + kind().box.x; }
    /**
     * Shown walking towards `feet`, without a step; true (and nothing shown) when its feet (as seen in the last frame)
     * are there.
     */
    bool faceTowards(int feet);
};

}  // namespace ugh::passengers::route
