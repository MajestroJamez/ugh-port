#include "physics/CopterPhysics.hpp"

#include <optional>

#include "physics/CollisionProbe.hpp"

namespace ugh::physics {

namespace {

using units::Fixed;
using units::Speed;
using Axis = CollisionProbe::Axis;

// speeds in 1/64 Fixed per frame; accelerations per frame
constexpr Speed MAX_SPEED = Speed::fromRaw(6144);
constexpr Speed WIND_PUSH = Speed::fromRaw(32);     // sideways, with the wind
constexpr Speed WIND_DOWN = Speed::fromRaw(16);     // and down
constexpr Speed STEER = Speed::fromRaw(63);         // the left or right key
constexpr Speed GRAVITY = Speed::fromRaw(27);
constexpr Speed DIVE = Speed::fromRaw(70);          // the down key
constexpr Speed LIFT = Speed::fromRaw(70);          // the up key
constexpr Speed WATER_BRAKE = Speed::fromRaw(193);  // sinking in the water
constexpr Speed BUOYANCY = Speed::fromRaw(21);      // floating up in the water

// effort (it spins the rotor) and energy, per frame
constexpr int STEER_EFFORT = 63, PEDAL_EFFORT = 90;
constexpr int FLYING_COST = 1, AIRBORNE_COST = 2, PEDAL_COST = 3;

// how far a copter can go (its top left corner)
constexpr Fixed LEFT_EDGE = Fixed::fromPixels(-16), RIGHT_EDGE = Fixed::fromPixels(304);
constexpr Fixed TOP_EDGE = Fixed::fromRaw(-608), BOTTOM_EDGE = Fixed::fromRaw(6112);

// from the copter's top left corner to the middle and the bottom of its skids, in pixels
constexpr int SKIDS_MIDDLE = 16, SKIDS_BOTTOM = 20;

}  // namespace

void CopterPhysics::fly(world::Copter& copter) {
    context_.level.energy().spend(FLYING_COST);
    copter.rotor().newFrame();
    impact_ = 0;
    Depth depth = depthOf(copter);
    if (!copter.landed()) {
        blowWithWind(copter, depth);
        steer(copter);
        moveHorizontally(copter);
    }
    liftAndFall(copter, depth);
    moveVertically(copter, depth);
    checkCrash(copter);
}

/** In the air the wind pushes the copter sideways and a little down. */
void CopterPhysics::blowWithWind(world::Copter& copter, Depth depth) {
    if (depth != Depth::Above || !context_.level.windy()) return;
    world::Motion& motion = copter.motion();
    Speed push = context_.level.wind() == data::levels::Wind::Left ? -WIND_PUSH : WIND_PUSH;
    motion.setSpeedX(motion.speedX() + push);
    motion.setSpeedY(motion.speedY() + WIND_DOWN);
}

/** The left or the right key. */
void CopterPhysics::steer(world::Copter& copter) {
    Speed vx = copter.motion().speedX();
    if (copter.controls().left) {
        vx -= STEER;
        copter.rotor().addEffort(STEER_EFFORT);
    } else if (copter.controls().right) {
        vx += STEER;
        copter.rotor().addEffort(STEER_EFFORT);
    }
    copter.motion().setSpeedX(vx.clamped(MAX_SPEED));
}

/** The move sideways against the collision mask, bouncing off walls. */
void CopterPhysics::moveHorizontally(world::Copter& copter) {
    world::Motion& motion = copter.motion();
    Speed vx = motion.speedX();
    Fixed target = motion.x() + vx.perFrame();
    if (target < LEFT_EDGE) {
        target = LEFT_EDGE;
        vx = Speed();
    } else if (target > RIGHT_EDGE) {
        target = RIGHT_EDGE;
        vx = Speed();
    }
    Fixed x = target;
    if (target.pixels() != motion.pixelX()) {
        std::optional<Fixed> stop =
            CollisionProbe(context_.level).stopOnTheWay(copter, Axis::Horizontal, motion.x(), target);
        if (stop) {
            x = *stop;
            impact_ = bounce(vx);
        }
    }
    motion.setSpeedX(vx);
    motion.moveToX(x);
}

/** Buoyancy in the water, gravity and diving in the air, pedalling up. */
void CopterPhysics::liftAndFall(world::Copter& copter, Depth depth) {
    Speed vy = copter.motion().speedY();
    bool canPedal = true;
    if (depth == Depth::Below) {
        context_.level.energy().spend(AIRBORNE_COST);
        if (vy > Speed()) {
            vy -= WATER_BRAKE;
        } else {
            vy -= BUOYANCY;
            copter.takeOff();
        }
        canPedal = false;
    } else if (depth == Depth::Above && !copter.landed()) {
        vy += GRAVITY;
        context_.level.energy().spend(AIRBORNE_COST);
        if (copter.controls().down) {
            vy += DIVE;
            copter.rotor().addEffort(PEDAL_EFFORT);
            canPedal = false;
        }
    }
    if (canPedal && copter.controls().up) {
        context_.level.energy().spend(PEDAL_COST);
        vy -= LIFT;
        copter.rotor().addEffort(PEDAL_EFFORT);
        copter.takeOff();
    }
    copter.motion().setSpeedY(vy.clamped(MAX_SPEED));
}

/** The move up or down against the collision mask; under water the copter floats up to the surface. */
void CopterPhysics::moveVertically(world::Copter& copter, Depth depth) {
    world::Motion& motion = copter.motion();
    Speed vy = motion.speedY();
    Fixed target = motion.y() + vy.perFrame();
    if (target < TOP_EDGE) {
        target = TOP_EDGE;
        vy = Speed();
    } else if (target > BOTTOM_EDGE) {
        target = BOTTOM_EDGE;
        vy = Speed();
    }
    int surface = context_.level.water().row();
    if (depth == Depth::Below && target.pixels() - surface + world::Copter::WATERLINE <= 0) {
        // floats up to the surface and stops there
        vy = Speed();
        target = Fixed::fromPixels(surface - world::Copter::WATERLINE);
    }
    motion.setSpeedY(vy);
    Fixed y = target;
    if (target.pixels() != motion.pixelY()) {
        std::optional<Fixed> stop =
            CollisionProbe(context_.level).stopOnTheWay(copter, Axis::Vertical, motion.y(), target);
        if (stop) {
            y = *stop;
            bounceVertically(copter, y);
        }
    }
    motion.moveToY(y);
}

/** The bounce off a floor or a ceiling; a soft one on a pad is a touch-down. */
void CopterPhysics::bounceVertically(world::Copter& copter, Fixed y) {
    Speed vy = copter.motion().speedY();
    int impact = bounce(vy);
    copter.motion().setSpeedY(vy);
    if (impact > impact_) impact_ = impact;
    if (vy >= Speed()) return;   // hit a ceiling
    if (impact_ >= context_.session.crashLimit()) return;
    touchDownOnPad(copter, y);
}

/** It lands when its skids are on the surface of a pad. */
void CopterPhysics::touchDownOnPad(world::Copter& copter, Fixed y) {
    int skidsY = y.pixels() + SKIDS_BOTTOM;
    int middle = copter.motion().pixelX() + SKIDS_MIDDLE;
    for (const world::Pad& pad : context_.level.pads()) {
        if (pad.place().y == skidsY && pad.place().spans(middle)) {
            copter.land(pad);
            return;
        }
    }
}

/** Too hard a bounce ends the attempt. */
void CopterPhysics::checkCrash(const world::Copter& copter) {
    if (impact_ < context_.session.crashLimit() || context_.level.fade().fadingOut()) return;
    context_.level.fade().startFadeOut();
    context_.report({events::EventKind::CopterCrashed, copter.player()});
}

int CopterPhysics::bounce(Speed& speed) {
    speed = (-speed) >> 1;
    int impact = speed.raw() << 1;
    return impact < 0 ? -impact : impact;
}

}  // namespace ugh::physics
