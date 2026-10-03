// The water and the rain (113b:2d1c, 2db9, 3c35, 3c78; Draw.kt), without the drawing.
#include "game.hpp"

namespace ugh {

namespace {

constexpr int ROW = CollisionMask::WIDTH;
constexpr int SCREEN_WIDTH = 320;

/** 113b:3c35 - Draw.kt spawnRaindrop: drop i starts again at a random place on the top edge or the windward side. */
void spawnRaindrop(World& world, int i) {
    uint16_t place = world.random.next(static_cast<uint16_t>(SCREEN_WIDTH + world.water.row));
    int x = 0, y = 0;
    if (place < SCREEN_WIDTH) {
        x = place;
    } else {
        y = place - SCREEN_WIDTH;
        x = world.wind == 1 ? SCREEN_WIDTH - 1 : 0;
    }
    world.rain.drops[i] = (y & 0xff) * ROW + x;
}

}  // namespace

/** 113b:2d1c - Draw.kt updateWater (without the drawing): the surface moves every second frame. */
void moveWater(World& world) {
    Water& w = world.water;
    if (w.hold != 0) {   // the frame after the row changed only redraws
        w.hold = 0;
        return;
    }
    int16_t row = w.row;
    w.toggle ^= 1;
    if (w.toggle == 0) {
        w.level += Fixed(world.level ? world.level->waterSpeed : 0);
        if (w.level < Fixed(0)) w.level = Fixed(0);
    }
    w.row = w.level.pixels();
    if (w.row != row) w.hold = static_cast<uint8_t>(w.hold - 1);
}

/** 113b:2db9 - Draw.kt drawWaterSurface (without the drawing): the surface animation, where the rain stops. */
void animateWaterSurface(World& world) {
    Water& w = world.water;
    w.surfaceDelay = static_cast<uint8_t>(w.surfaceDelay - 1);
    if (w.surfaceDelay & 0x80) {
        w.surfaceDelay = 6;
        w.surfaceFrame = static_cast<int16_t>(w.surfaceFrame - 1);
        if (w.surfaceFrame < 0) w.surfaceFrame = 2;
    }
    world.rain.floorRow = static_cast<uint8_t>(w.row);
}

/** 113b:3976 (the end of loadLevel) - a windy level starts with the rain already falling. */
void startRain(Game& game) {
    World& world = game.world;
    if (world.wind == 0) return;
    for (int i = Rain::DROPS - 1; i >= 0; i--) spawnRaindrop(world, i);
    for (int i = 0; i < 0x241; i++) moveRain(game);
}

/**
 * 113b:3c78 - Draw.kt moveRain: the drops down to the water fall diagonally with the wind, the even ones 3 px a
 * frame, the odd ones 2 px; a drop at the water starts again.
 */
void moveRain(Game& game) {
    World& world = game.world;
    int last = world.water.row + 1;
    if (last >= Rain::DROPS) {
        game.problems.push_back("water below the screen: the original moves raindrops past the last one");
        last = Rain::DROPS - 1;
    }
    int direction = world.windDirection();
    int floor = world.rain.floorRow * (ROW / 4);   // in the bytes of the VGA page, like the original compares
    for (int i = last; i >= 0; i--) {
        int step = i % 2 == 0 ? 3 : 2;
        int drop = world.rain.drops[i] + step * ROW + step * direction;
        if (static_cast<uint16_t>(drop >> 2) >= floor) spawnRaindrop(world, i);
        else world.rain.drops[i] = drop;
    }
}

}  // namespace ugh
