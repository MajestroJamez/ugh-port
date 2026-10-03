// A pad of a level.
#pragma once

#include "units/Int16.hpp"

namespace ugh::data {

/** A pad of a level, in pixels. */
struct PadDefinition {
    units::Int16 left, right;   // the landing area
    units::Int16 y;             // the surface
    units::Int16 door;          // where passengers come out and go in
    units::Int16 wait;          // where they wait
    units::Int16 stand;         // where an enemy or the standing passenger stands
    units::Int16 number;        // shown in the bubbles and the status line

    /** x is over the landing area. */
    bool spans(units::Int16 x) const { return x >= left && x <= right; }
};

}  // namespace ugh::data
