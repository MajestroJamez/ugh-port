// Where a moving copter hits the background.
#pragma once

#include <optional>

#include "core/Fixed.hpp"
#include "model/Copter.hpp"
#include "model/Level.hpp"

namespace ugh::physics {

/**
 * 113b:1457 - Game.kt probe: ten points of the copter's outline tested against the collision mask of the level.
 * The physics moves a copter pixel by pixel along one axis and stops it at the first pixel where a point of the
 * outline would be inside something solid.
 *
 * Quirk of the original: moving left or up, it probes only the pixel next to the copter, however far the copter
 * moves this frame (its loop does not advance the probe). A fast copter can thus pass through a thin wall going left
 * or up, never going right or down.
 */
class CollisionProbe {
public:
    enum class Axis { Horizontal, Vertical };

    explicit CollisionProbe(const model::Level& level) : level_(level) {}

    /**
     * The copter moves along `axis` from `from` (its position on that axis now) to `to`: where it stops when it
     * hits something on the way, nothing when it gets there.
     */
    std::optional<core::Fixed> stopOnTheWay(const model::Copter& copter, Axis axis, core::Fixed from,
                                            core::Fixed to) const;

private:
    const model::Level& level_;
    mutable int auditX_ = 0;

    /** A point of the outline around `origin` (a pixel index of the mask) is solid. */
    bool hits(int origin) const;
    /** The probe origin of a copter: its pixel position, 5 px to the right, as a pixel index of the mask. */
    static int originOf(const model::Copter& copter);
};

}  // namespace ugh::physics
