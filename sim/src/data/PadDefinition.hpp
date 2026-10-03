// A pad of a level.
#pragma once

#include "core/Word.hpp"

namespace ugh::data {

/** A pad (level list A), in pixels. */
struct PadDefinition {
    core::Word left, right;   // the landing area
    core::Word y;             // the surface
    core::Word doorX;         // where passengers come out and go in
    core::Word waitX;         // where they wait
    core::Word standX;        // where an enemy or the standing passenger stands
    core::Word number;        // shown in the bubbles and the status line
};

}  // namespace ugh::data
