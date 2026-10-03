// What the states of a walking passenger on its pickup pad share.
#pragma once

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/**
 * The states of a walking passenger on its pickup pad (Waiting, Calling, Impatient, Boarding): the rising water can
 * reach it there, and a copter flying through it knocks it into the water. Either way it turns into its swimming
 * kind and splashes in.
 */
class OnPickupPad : public PassengerState {
protected:
    /** 113b:15d7 .. 18e6 - the water reached the passenger's knees: it is in. */
    static bool fellIntoWater(model::Passenger& passenger, model::Level& level);

    /** 113b:2276 - a copter in the air flew into the passenger: it is in the water. */
    static bool knockedIntoWater(model::Passenger& passenger, model::Level& level);

private:
    static void intoWater(model::Passenger& passenger, model::Level& level);
};

}  // namespace ugh::passengers
