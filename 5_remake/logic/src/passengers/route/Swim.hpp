// A passenger in the water.
#pragma once

#include "units/Countdown.hpp"
#include "units/Speed.hpp"

namespace ugh::passengers::route {

/** A passenger in the water: its speed going under and up again, and how long it stays afloat. */
class Swim {
public:
    /** It falls into the water, or starts to sink: no speed yet. */
    void plunge() { speed_ = units::Speed(); }
    /** One frame of the splash above the surface: it falls faster; the new speed. */
    units::Speed fallInAir();
    /** One frame of the splash under the surface: braked while it still goes down, then up again; the new speed. */
    units::Speed brakeInWater();
    /** One frame of sinking, faster and faster; the new speed. */
    units::Speed sink();
    /** 1/64 Fixed per frame; positive: down. */
    units::Speed speed() const { return speed_; }

    /** It floats on the surface for `frames`. */
    void startAfloat(int frames) { afloat_.start(frames); }
    /** One frame afloat; true when the time is up. */
    bool tickAfloat() { return afloat_.tick(); }
    int afloatTime() const { return afloat_.remaining(); }

private:
    units::Speed speed_;
    units::Countdown afloat_;
};

}  // namespace ugh::passengers::route
