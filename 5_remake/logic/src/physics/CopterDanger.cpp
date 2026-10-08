#include "physics/CopterDanger.hpp"

#include <algorithm>

#include "physics/CollisionProbe.hpp"
#include "physics/CopterPhysics.hpp"

namespace ugh::physics {

namespace {

using units::Speed;
using world::copter::CopterShape;

constexpr int SUBPIXELS = units::Fixed::fromPixels(1).raw();

/** How far (pixels, rounded up) a copter sinking at `speed` under water goes while a bounce would still crash it. */
int brakingPixels(Speed speed, int crashLimit) {
    int way = 0;   // 1/32 px
    for (;;) {
        speed -= CopterPhysics::WATER_BRAKE;
        if (speed <= Speed() || CopterPhysics::impactOf(speed) < crashLimit) break;
        way += speed.perFrame().raw();
    }
    return (way + SUBPIXELS - 1) / SUBPIXELS;
}

}  // namespace

CopterDanger CopterDanger::of(const world::Level& level, int crashLimit, const world::copter::Copter& copter) {
    using Probe = CollisionProbe;
    const world::copter::Motion& motion = copter.motion();
    const Probe probe(level);
    CopterDanger danger;
    danger.crashLimit = crashLimit;

    // across: to the edge of the screen
    const Speed vx = motion.speedX();
    danger.across = {vx, CopterPhysics::impactOf(vx), false};
    if (vx != Speed()) {
        const int direction = vx > Speed() ? 1 : -1;
        const int most = direction > 0 ? CopterPhysics::RIGHT_EDGE.pixels() - motion.pixelX()
                                       : motion.pixelX() - CopterPhysics::LEFT_EDGE.pixels();
        danger.across.rock = most > 0 && probe.clearance(copter, Probe::Axis::Horizontal, direction, most).has_value();
    }

    // up and down: up to the top edge (under water to the surface), down to the water and under it as far as it is
    // braked from a crash
    const Speed vy = motion.speedY();
    danger.upDown = {vy, CopterPhysics::impactOf(vy), false};
    const int surface = level.water().row();
    const int floating = CopterShape::floatingY(surface);   // its top when its waterline is at the surface
    const bool under = copter.depthIn(surface) > 0;
    int most = 0;
    if (vy < Speed()) {
        most = motion.pixelY() - (under ? floating : CopterPhysics::TOP_EDGE.pixels());
    } else if (vy > Speed()) {
        const int toWater = std::max(floating - motion.pixelY(), 0);
        most = std::min(toWater + brakingPixels(vy, crashLimit),
                        CopterPhysics::BOTTOM_EDGE.pixels() - motion.pixelY());
    }
    if (most > 0) {
        const int direction = vy > Speed() ? 1 : -1;
        danger.upDown.rock = probe.clearance(copter, Probe::Axis::Vertical, direction, most).has_value();
    }
    return danger;
}

}  // namespace ugh::physics
