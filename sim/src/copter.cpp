// The copters: physics with the collision probe, landing, crash, the rotor (113b:1095 .. 1485, 418d; Game.kt).
#include <algorithm>
#include <array>
#include <cstdlib>
#include <utility>

#include "game.hpp"

namespace ugh {

namespace {

constexpr int ROW = CollisionMask::WIDTH;

/**
 * 113b:1457 - Game.kt probe: the points of the copter's outline that hit the background, around the probe origin
 * (x 0 / 12 / 20, y 0 / 6 / 12 / 19 pixels from it).
 */
constexpr std::array<std::pair<int, int>, 10> PROBE = {
    {{0, 0}, {12, 0}, {20, 0}, {0, 19}, {12, 19}, {20, 19}, {0, 6}, {20, 6}, {0, 12}, {20, 12}}};

constexpr int16_t MAX_SPEED = 0x1800;

bool probe(const World& world, int origin) {
    if (!world.level) return false;
    for (auto [dx, dy] : PROBE)
        if (world.level->mask.solid(origin + dy * ROW + dx)) return true;
    return false;
}

/** The probe origin of a copter: its pixel position, 5 px to the right. */
int probeOrigin(const Copter& c) { return c.pixelY * ROW + c.pixelX + 5; }

/** Where the copter is relative to the water surface (the high byte of the depth in the original). */
enum class Depth { Above, Surface, Below };

Depth depthOf(const World& world, const Copter& c) {
    auto depth = static_cast<int16_t>(c.pixelY - world.water.row + 0x12);
    return depth < 0 ? Depth::Above : depth == 0 ? Depth::Surface : Depth::Below;
}

/** A bounce: half the speed back; the impact is the speed it had (doubled half, as the original computes it). */
int16_t bounce(int16_t& speed) {
    speed = static_cast<int16_t>(static_cast<int16_t>(-speed) >> 1);
    auto impact = static_cast<int16_t>(speed * 2);
    return impact < 0 ? static_cast<int16_t>(-impact) : impact;
}

/** Game.kt moveHorizontally: wind, left / right, movement with the collision probe, bouncing off walls. */
void moveHorizontally(World& world, Copter& c, Depth depth) {
    if (depth == Depth::Above && world.wind != 0) {
        auto push = static_cast<int16_t>(world.windDirection() * 32);
        c.vx = static_cast<int16_t>(c.vx + push);
        c.vy = static_cast<int16_t>(c.vy + std::abs(push >> 1));
    }
    int16_t vx = c.vx;
    if (c.keys.left) {
        vx = static_cast<int16_t>(vx - 0x3f);
        c.effort = static_cast<int16_t>(c.effort + 0x3f);
    } else if (c.keys.right) {
        vx = static_cast<int16_t>(vx + 0x3f);
        c.effort = static_cast<int16_t>(c.effort + 0x3f);
    }
    c.vx = std::clamp<int16_t>(vx, -MAX_SPEED, MAX_SPEED);

    Fixed target = c.x + perFrame(c.vx);
    if (target < Fixed(-0x200)) { target = Fixed(-0x200); c.vx = 0; }
    else if (target > Fixed(0x2600)) { target = Fixed(0x2600); c.vx = 0; }

    Fixed x = target;
    if (target.pixels() != c.pixelX) {
        bool hit = false;
        int origin = probeOrigin(c);
        if (target <= c.x) {
            // moving left the original probes only the pixel next to the copter, however far it moves (its loop
            // does not advance the probe)
            if (probe(world, origin - 1)) { x = c.x.wholePixel() + Fixed::fromPixels(1); hit = true; }
        } else {
            for (x = c.x;; x += Fixed::fromPixels(1)) {
                if (probe(world, ++origin)) { x = x.wholePixel(); hit = true; break; }
                if (x + Fixed::fromPixels(1) >= target) break;
            }
        }
        if (hit) c.impact = bounce(c.vx); else x = target;
    }
    c.x = x;
    c.pixelX = x.pixels();
}

/** Game.kt bounceVertically: bounce, impact, a soft touch-down on the pad under the copter. */
void bounceVertically(Game& game, int player, Fixed y) {
    Copter& c = game.world.copters[player];
    int16_t impact = bounce(c.vy);
    if (impact > c.impact) c.impact = impact;
    if (c.vy >= 0) return;   // hit a ceiling
    if (c.impact >= game.data.crashLimit(game.world.difficulty)) return;
    auto padY = static_cast<int16_t>(y.pixels() + 0x14);
    auto middle = static_cast<int16_t>(c.pixelX + 0x10);
    for (int i = 0; i < game.world.padCount; i++) {
        const Pad& pad = game.world.pads[i];
        if (pad.y == padY && middle >= pad.left && middle <= pad.right) {
            c.landedPad = static_cast<int16_t>(i);
            c.vx = 0;
            c.vy = 0;
            return;
        }
    }
}

/** Game.kt moveVertically: buoyancy, gravity, diving, pedalling, movement with the probe, crash. */
void moveVertically(Game& game, int player, Depth depth) {
    World& world = game.world;
    Copter& c = world.copters[player];
    int16_t vy = c.vy;
    bool pedal = true;
    if (depth == Depth::Below) {   // buoyancy
        world.energy = static_cast<int16_t>(world.energy - 2);
        if (vy > 0) {
            vy = static_cast<int16_t>(vy - 0xc1);
        } else {
            vy = static_cast<int16_t>(vy - 0x15);
            c.landedPad = -1;
        }
        pedal = false;
    } else if (depth == Depth::Above && !c.landed()) {   // gravity, diving
        vy = static_cast<int16_t>(vy + 0x1b);
        world.energy = static_cast<int16_t>(world.energy - 2);
        if (c.keys.down) {
            vy = static_cast<int16_t>(vy + 0x46);
            c.effort = static_cast<int16_t>(c.effort + 0x5a);
            pedal = false;
        }
    }
    if (pedal && c.keys.up) {
        world.energy = static_cast<int16_t>(world.energy - 3);
        vy = static_cast<int16_t>(vy - 0x46);
        c.effort = static_cast<int16_t>(c.effort + 0x5a);
        c.landedPad = -1;
    }
    c.vy = std::clamp<int16_t>(vy, -MAX_SPEED, MAX_SPEED);

    Fixed target = c.y + perFrame(c.vy);
    if (target < Fixed(-0x260)) { target = Fixed(-0x260); c.vy = 0; }
    else if (target > Fixed(0x17e0)) { target = Fixed(0x17e0); c.vy = 0; }
    if (depth == Depth::Below && static_cast<int16_t>(target.pixels() - world.water.row + 0x12) <= 0) {
        // floats up to the surface and stops there
        c.vy = 0;
        target = Fixed::fromPixels(world.water.row) - Fixed(0x240);
    }

    Fixed y = target;
    if (target.pixels() != c.pixelY) {
        bool hit = false;
        int origin = probeOrigin(c);
        if (target <= c.y) {
            // moving up, like moving left: only the row above the copter is probed
            if (probe(world, origin - ROW)) { y = c.y.wholePixel() + Fixed::fromPixels(1); hit = true; }
        } else {
            for (y = c.y;; y += Fixed::fromPixels(1)) {
                if (probe(world, origin += ROW)) { y = y.wholePixel(); hit = true; break; }
                if (y + Fixed::fromPixels(1) >= target) break;
            }
        }
        if (hit) bounceVertically(game, player, y); else y = target;
    }
    c.y = y;
    c.pixelY = y.pixels();

    if (c.impact >= game.data.crashLimit(world.difficulty) && !world.fade.fadingOut()) {
        world.fade.startFadeOut();
        game.report({EventKind::CopterCrashed, player});
    }
}

}  // namespace

/** 113b:1095 - Game.kt copterUpdate: physics of the copter of a player. */
void flyCopter(Game& game, int player) {
    World& world = game.world;
    Copter& c = world.copters[player];
    world.energy = static_cast<int16_t>(world.energy - 1);
    c.impact = 0;
    c.effort = 0;
    Depth depth = depthOf(world, c);
    if (!c.landed()) moveHorizontally(world, c, depth);
    moveVertically(game, player, depth);
}

/** 113b:418d - Draw.kt drawCopter (without the drawing): the rotor, spinning faster with the effort. */
void spinRotor(Game& game, int player) {
    Copter& c = game.world.copters[player];
    c.rotorCounter = static_cast<int16_t>(c.rotorCounter - std::min<int16_t>(c.effort, 0x5a));
    if (c.rotorCounter >= 0) return;
    c.rotorCounter = static_cast<int16_t>(c.rotorCounter + 0xc8);
    auto next = static_cast<Sprite>(c.rotor + 1);
    if (static_cast<int16_t>(next) >= static_cast<int16_t>(game.data.rotorEnd(player))) next = game.data.rotorFirst(player);
    c.rotor = next;
}

int touchingCopter(const World& world, const Box& box, Fixed x, Fixed y) {
    // the box of the sprite against the copter's corner, with the size of the copter in it
    Fixed bottom = y + Fixed::fromPixels(box.y);
    Fixed top = y + Fixed::fromPixels(box.y - 2 * box.halfHeight) - Fixed::fromPixels(20);
    Fixed right = x + Fixed::fromPixels(box.x + box.halfWidth) - Fixed::fromPixels(5);
    Fixed left = x + Fixed::fromPixels(box.x - box.halfWidth) - Fixed::fromPixels(26);
    for (int p = 0; p < world.copterCount(); p++) {
        const Copter& c = world.copters[p];
        if (bottom >= c.y && top <= c.y && right >= c.x && left <= c.x) return p;
    }
    return -1;
}

}  // namespace ugh
