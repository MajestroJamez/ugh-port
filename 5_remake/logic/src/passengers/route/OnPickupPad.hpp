// A state of a passenger on its pickup pad.
#pragma once

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/RouteState.hpp"

namespace ugh::passengers::route {

/**
 * A state of a passenger on its pickup pad (Waiting, Calling, Impatient, Boarding; Template Method): every frame it
 * falls into the water when the water reaches the middle of its body, and a copter in the air that touches it knocks
 * it in.
 */
class OnPickupPad : public RouteState {
public:
    void update(RoutePassenger& passenger, const PassengerContext& context) const final;

protected:
    /** What it does before a copter can knock it into the water (Waiting: the walk to its spot). */
    virtual void walk(RoutePassenger&, const PassengerContext&) const {}
    /** The rest of its frame on the pad. */
    virtual void stay(RoutePassenger& passenger, const PassengerContext& context) const = 0;

private:
    static bool fellIntoWater(RoutePassenger& passenger, const PassengerContext& context);
    static bool knockedIntoWater(RoutePassenger& passenger, const PassengerContext& context);
    static void intoWater(RoutePassenger& passenger, const PassengerContext& context);
};

}  // namespace ugh::passengers::route
