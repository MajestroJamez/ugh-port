#include "physics/CopterPhysics.hpp"

#include <optional>

#include "physics/CollisionProbe.hpp"

namespace ugh::physics {

namespace {

using core::Fixed;
using core::Speed;
using core::Word;
using Axis = CollisionProbe::Axis;

// speeds in 1/64 Fixed per frame, accelerations per frame
constexpr Speed MAX_SPEED(0x1800);
constexpr Speed WIND_PUSH(0x20);      // sideways, with the wind
constexpr Speed WIND_DOWN(0x10);      // and down
constexpr Speed STEER(0x3f);          // left / right key
constexpr Speed GRAVITY(0x1b);
constexpr Speed DIVE(0x46);           // down key
constexpr Speed LIFT(0x46);           // up key
constexpr Speed WATER_BRAKE(0xc1);    // sinking in the water
constexpr Speed BUOYANCY(0x15);       // floating up in the water

// effort (spins the rotor) and energy per frame
constexpr Word STEER_EFFORT = 0x3f, PEDAL_EFFORT = 0x5a;
constexpr Word FLYING_COST = 1, AIRBORNE_COST = 2, PEDAL_COST = 3;

// how far a copter can go (its top left corner)
constexpr Fixed LEFT_EDGE(-0x200), RIGHT_EDGE(0x2600), TOP_EDGE(-0x260), BOTTOM_EDGE(0x17e0);

// from the copter's top left corner to the middle and the bottom of its skids, in pixels
constexpr Word SKIDS_MIDDLE = 0x10, SKIDS_BOTTOM = 0x14;

constexpr uint8_t WIND_TO_THE_LEFT = 1;

}  // namespace

void CopterPhysics::fly(int player) {
    model::Copter& copter = level_.copter(player);
    level_.energy().spend(FLYING_COST);
    copter.startFrame();
    Depth depth = depthOf(copter);
    if (!copter.landed()) {
        blowWithWind(copter, depth);
        steer(copter);
        moveHorizontally(copter);
    }
    liftAndFall(copter, depth);
    moveVertically(copter, depth);
    checkCrash(copter, player);
}

CopterPhysics::Depth CopterPhysics::depthOf(const model::Copter& copter) const {
    Word depth = copter.depthIn(level_.water().row());
    return depth < 0 ? Depth::Above : depth == 0 ? Depth::Surface : Depth::Below;
}

/** Game.kt moveHorizontally (start): in the air the wind pushes the copter sideways and a little down. */
void CopterPhysics::blowWithWind(model::Copter& copter, Depth depth) {
    if (depth != Depth::Above || !level_.windy()) return;
    Speed push = level_.wind() == WIND_TO_THE_LEFT ? -WIND_PUSH : WIND_PUSH;
    copter.setSpeed(copter.speedX() + push, copter.speedY() + WIND_DOWN);
}

/** Game.kt moveHorizontally: the left or the right key. */
void CopterPhysics::steer(model::Copter& copter) {
    Speed vx = copter.speedX();
    if (copter.controls().left) {
        vx -= STEER;
        copter.addEffort(STEER_EFFORT);
    } else if (copter.controls().right) {
        vx += STEER;
        copter.addEffort(STEER_EFFORT);
    }
    copter.setSpeed(vx.clamped(MAX_SPEED), copter.speedY());
}

/** Game.kt moveHorizontally: the move with the collision probe, bouncing off walls. */
void CopterPhysics::moveHorizontally(model::Copter& copter) {
    Speed vx = copter.speedX();
    Fixed target = copter.x() + vx.perFrame();
    if (target < LEFT_EDGE) { target = LEFT_EDGE; vx = Speed(0); }
    else if (target > RIGHT_EDGE) { target = RIGHT_EDGE; vx = Speed(0); }

    Fixed x = target;
    if (target.pixels() != copter.pixelX()) {
        std::optional<Fixed> stop = CollisionProbe(level_).stopOnTheWay(copter, Axis::Horizontal, copter.x(), target);
        if (stop) {
            x = *stop;
            copter.setImpact(bounce(vx));
        }
    }
    copter.setSpeed(vx, copter.speedY());
    copter.moveToX(x);
}

/** Game.kt moveVertically: buoyancy in the water, gravity and diving in the air, pedalling up. */
void CopterPhysics::liftAndFall(model::Copter& copter, Depth depth) {
    Speed vy = copter.speedY();
    bool canPedal = true;
    if (depth == Depth::Below) {
        level_.energy().spend(AIRBORNE_COST);
        if (vy > Speed(0)) {
            vy -= WATER_BRAKE;
        } else {
            vy -= BUOYANCY;
            copter.takeOff();
        }
        canPedal = false;
    } else if (depth == Depth::Above && !copter.landed()) {
        vy += GRAVITY;
        level_.energy().spend(AIRBORNE_COST);
        if (copter.controls().down) {
            vy += DIVE;
            copter.addEffort(PEDAL_EFFORT);
            canPedal = false;
        }
    }
    if (canPedal && copter.controls().up) {
        level_.energy().spend(PEDAL_COST);
        vy -= LIFT;
        copter.addEffort(PEDAL_EFFORT);
        copter.takeOff();
    }
    copter.setSpeed(copter.speedX(), vy.clamped(MAX_SPEED));
}

/** Game.kt moveVertically: the move with the collision probe; under water it floats up to the surface. */
void CopterPhysics::moveVertically(model::Copter& copter, Depth depth) {
    Speed vy = copter.speedY();
    Fixed target = copter.y() + vy.perFrame();
    if (target < TOP_EDGE) { target = TOP_EDGE; vy = Speed(0); }
    else if (target > BOTTOM_EDGE) { target = BOTTOM_EDGE; vy = Speed(0); }
    Word surface = level_.water().row();
    if (depth == Depth::Below && target.pixels() - surface + model::Copter::WATERLINE <= 0) {
        // floats up to the surface and stops there
        vy = Speed(0);
        target = Fixed::fromPixels(surface - model::Copter::WATERLINE);
    }
    copter.setSpeed(copter.speedX(), vy);

    Fixed y = target;
    if (target.pixels() != copter.pixelY()) {
        std::optional<Fixed> stop = CollisionProbe(level_).stopOnTheWay(copter, Axis::Vertical, copter.y(), target);
        if (stop) {
            y = *stop;
            bounceVertically(copter, y);
        }
    }
    copter.moveToY(y);
}

/** Game.kt bounceVertically: the bounce off a floor or a ceiling; a soft one on a pad is a touch-down. */
void CopterPhysics::bounceVertically(model::Copter& copter, Fixed y) {
    Speed vy = copter.speedY();
    Word impact = bounce(vy);
    copter.setSpeed(copter.speedX(), vy);
    if (impact > copter.impact()) copter.setImpact(impact);
    if (vy >= Speed(0)) return;   // hit a ceiling
    if (copter.impact() >= level_.session().crashLimit()) return;
    touchDownOnPad(copter, y);
}

/** Game.kt bounceVertically: lands when its skids are on the surface of a pad. */
void CopterPhysics::touchDownOnPad(model::Copter& copter, Fixed y) {
    Word skidsY = y.pixels() + SKIDS_BOTTOM;
    Word middle = copter.pixelX() + SKIDS_MIDDLE;
    for (int i = 0; i < level_.padCount(); i++) {
        const model::Pad& pad = level_.pad(i);
        if (pad.y() == skidsY && pad.spans(middle)) {
            copter.land(i);
            return;
        }
    }
}

/** Game.kt moveVertically (end): too hard an impact ends the attempt. */
void CopterPhysics::checkCrash(const model::Copter& copter, int player) {
    if (copter.impact() < level_.session().crashLimit() || level_.fade().fadingOut()) return;
    level_.fade().startFadeOut();
    level_.report({core::EventKind::CopterCrashed, player});
}

Word CopterPhysics::bounce(Speed& speed) {
    speed = (-speed) >> 1;
    Word impact = speed.raw() << 1;
    return impact < 0 ? -impact : impact;
}

}  // namespace ugh::physics
