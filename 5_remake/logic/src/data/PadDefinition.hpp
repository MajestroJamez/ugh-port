// A pad of a level.
#pragma once

namespace ugh::data {

/** A pad of a level, in pixels. */
struct PadDefinition {
    int left = 0, right = 0;   // the landing area
    int y = 0;                 // the surface
    int door = 0;              // where passengers come out and go in
    int wait = 0;              // where they wait
    int stand = 0;             // where an enemy or the standing passenger stands
    int number = 0;            // shown in the bubbles and the status line

    /** x is over the landing area. */
    bool spans(int x) const { return x >= left && x <= right; }
};

}  // namespace ugh::data
