// The water of a level.
#pragma once

#include "units/Fixed.hpp"

namespace ugh::world::scenery {

/**
 * The water at the bottom of a level. Its surface moves every second frame; in the frame after its pixel row
 * changed it rests. The surface animation runs SURFACE_LAST_FRAME .. 0 and again, a frame of it every SURFACE_DELAY + 1
 * frames. The delay is counted by hand, not by a `units::Countdown`: it runs SURFACE_DELAY .. 0 and acts when it would
 * go below 0, the field the replays show; a countdown acts at 0 and starts again above it.
 */
class Water {
public:
    /** A new attempt: the surface of the level, the counters from the start. */
    void fill(units::Fixed level);

    /** One frame: the surface moves by `speed` every second frame (negative: it rises), never above the top. */
    void move(units::Fixed speed);

    /** One frame of the surface animation. */
    void animateSurface();

    units::Fixed level() const { return level_; }
    /** The pixel row of the surface. */
    int row() const { return level_.pixels(); }
    bool resting() const { return resting_; }
    /** 0 in the frames the surface moves, 1 in the others. */
    int evenFrame() const { return evenFrame_; }
    int surfaceFrame() const { return surfaceFrame_; }
    int surfaceDelay() const { return surfaceDelay_; }

private:
    static constexpr int SURFACE_DELAY = 6;
    static constexpr int SURFACE_LAST_FRAME = 2;

    units::Fixed level_;
    bool resting_ = false;
    int evenFrame_ = 0;
    int surfaceFrame_ = 0;
    int surfaceDelay_ = 0;
};

}  // namespace ugh::world::scenery
