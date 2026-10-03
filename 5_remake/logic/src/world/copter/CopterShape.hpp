// The shape of a copter.
#pragma once

#include <array>

namespace ugh::world::copter {

/**
 * The shape of a copter: its points that touch the world, in pixels from its top left corner (where it is). Points
 * that only happen to lie in the same column keep their own names: they mean different things.
 */
class CopterShape {
public:
    /** A point of the copter, in pixels from its top left corner. */
    struct Point {
        int x, y;
    };

    /** Where a passenger of a route gets in and out: the column it walks to. */
    static constexpr int DOOR_X = 16;

    /** Where what the copter lets go starts to fall: the standing passenger below it, a quick delivery's bonus item. */
    static constexpr Point DROP = {16, 10};

    /** The middle and the bottom of its skids: it lands when they are on the surface of a pad. */
    static constexpr Point SKIDS = {16, 20};

    /** From its top to its waterline: it floats when the waterline is at the water surface. */
    static constexpr int WATERLINE = 18;

    /** Its body, which a sprite touches: 5 .. 26 across, 20 high. */
    static constexpr int BODY_LEFT = 5, BODY_RIGHT = 26, BODY_HEIGHT = 20;

    /** The outline the collision probe tests against the background starts this far right of its left edge. */
    static constexpr int OUTLINE_LEFT = 5;
    /** The points of the outline, in pixels from its start (OUTLINE_LEFT) and the top: the corners and the sides. */
    static constexpr std::array<Point, 10> OUTLINE = {
        {{0, 0}, {12, 0}, {20, 0}, {0, 19}, {12, 19}, {20, 19}, {0, 6}, {20, 6}, {0, 12}, {20, 12}}};
};

}  // namespace ugh::world::copter
