// The water of a level.
#pragma once

#include <cstdint>

#include "core/Fixed.hpp"
#include "core/Word.hpp"

namespace ugh::model {

/** The water at the bottom of the level: its surface rises or sinks every second frame. */
class Water {
public:
    struct Snapshot {
        core::Fixed level;          // the surface (28fe)
        core::Word row;             // the surface in whole pixels (2903)
        uint8_t hold = 0;           // a frame without movement after the row changed (27a2)
        uint8_t toggle = 0;         // every second frame (27ce)
        core::Word surfaceFrame;    // the animation of the surface, 2 .. 0 (27a4)
        uint8_t surfaceDelay = 0;   //   and its countdown (27a3)
    };

    /** A new level attempt: the counters start again (113b:3d66). */
    void startAttempt();

    /** The surface of the level being loaded (113b:3976). */
    void fillTo(core::Fixed level);

    /** 113b:2d1c - Draw.kt updateWater (without the drawing): the surface moves by `speed` every second frame. */
    void move(core::Word speed);

    /** 113b:2db9 - Draw.kt drawWaterSurface (without the drawing): the animation of the surface. */
    void animateSurface();

    core::Fixed level() const { return s_.level; }
    core::Word row() const { return s_.row; }

    /** The caption screen keeps the row of the surface elsewhere for the time it is shown (113b:0664). */
    void setRow(core::Word row) { s_.row = row; }

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    Snapshot s_;
};

}  // namespace ugh::model
