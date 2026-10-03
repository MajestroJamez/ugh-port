// A kind of passenger.
#pragma once

#include <string>

#include "data/Animation.hpp"
#include "data/AnimationPair.hpp"
#include "data/Box.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/**
 * A kind of passenger. A passenger with a route rides between the pads of its route and turns into the water kind of
 * its kind when it falls into the water (and back when it is rescued); the standing passenger waits on its pad to be
 * carried anywhere.
 */
struct PassengerKind {
    enum class Type { Route, Water, Standing };

    std::string name;
    Type type = Type::Route;
    Box box;
    const Animation* standing = nullptr;    // in the water: treading water
    const Animation* waving = nullptr;
    AnimationPair walking;                  // in the water: swimming
    const Animation* comingOut = nullptr;   // out of the door (route kinds)
    const Animation* goingIn = nullptr;     // into the door (route kinds)
    units::Int16 animDelay;                 // frames per animation frame
    units::Int16 fare, fareMin;             // the fare when boarding drops every frame of the ride to the minimum
    units::Int16 swimTime;                  // frames a swimmer stays afloat (water kinds)
    units::Int16 look;                      // who sits in the copter
    const PassengerKind* other = nullptr;   // the water kind of a route kind and back
    bool rescuable = true;                  // a copter on the water can pick the swimmer up (water kinds)
};

}  // namespace ugh::data
