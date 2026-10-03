// The game state: a typed model of what the original keeps in its data segment, without what only the drawing
// needs (where a sprite was drawn last frame, the status line, the VGA pages).
//
// The entities live in fixed slots like in the original (2 copters, 10 pads, 16 passengers, 5 enemies, 12 bonus
// items) and a level load sets only what the original sets: a value the load leaves alone stays from the slot's
// previous use, and the golden replays show it. Some words of the original serve two purposes depending on the
// state; the model keeps them as one field and says so.
#pragma once

#include <array>
#include <cstdint>
#include <variant>

#include "data.hpp"
#include "fixed.hpp"
#include "random.hpp"

namespace ugh {

/** The keys of a player that are held down (key states 278c .. 279e). */
struct Controls {
    bool up = false, down = false, left = false, right = false, fire = false;
};

/** A copter (player 0 or 1). */
struct Copter {
    Fixed x, y;                 // top left
    int16_t pixelX = 0;         // x, y in whole pixels: the physics updates them with x, y, but a walker that throws
    int16_t pixelY = 0;         //   the copter moves only y, so pixelY is behind until the next frame
    int16_t vx = 0, vy = 0;     // speed in 1/64 of the fixed point per frame (perFrame)
    int16_t landedPad = -1;     // the pad the copter stands on, -1 = in the air
    int16_t effort = 0;         // pedalling this frame (makes the rotor spin faster)
    int16_t impact = 0;         // hardest bounce this frame; crashes over GameData::crashLimit
    int16_t carrying = 0;       // look of the passenger on board (PassengerKind::look), 0 = nobody
    int16_t targetPad = 0;      // number of the pad the passenger wants to go to, 0 = none
    int16_t fare = 0;           // what the passenger pays now, drops every frame to
    int16_t fareMin = 0;        //   this minimum
    Sprite rotor = 0;           // rotor animation: current sprite
    int16_t rotorCounter = 0;   //   and the countdown to the next one
    Controls keys;

    bool landed() const { return landedPad >= 0; }
};

/** A passenger (level list B). */
struct Passenger {
    const PassengerKind* kind = nullptr;   // changes to its swimming kind in the water and back
    const PassengerState* state = nullptr;
    int16_t startPad = 0;
    RouteCursor route;
    int16_t pickupPad = 0, targetPad = 0;
    // a countdown in the calling and waving states, 1 / 0 = standing at / walking to the waiting spot, and the
    // carrying copter (player * 2) while riding or hanging: one word in the original (2a2d)
    int16_t counter = 0;
    // the delay before appearing, the x speed when dropped from a copter, the time afloat (2acd)
    int16_t timer = 0;
    Fixed x, y;                 // top left
    int16_t pixelX = 0;         // x, y in whole pixels as of the end of the last frame; the states read these
    int16_t pixelY = 0;
    // the y speed: in 1/32 px per frame when falling, in 1/64 of that (perFrame) in the water (2c6d)
    int16_t vy = 0;
    int16_t anim = 0, animDelay = 0;   // animation: frame index (-1 = before the first) and the countdown
    Sprite sprite = NO_SPRITE;
    Sprite bubble = NO_SPRITE;  // the speech bubble over the passenger
    int16_t bonusTimer = 0;     // a delivery before it runs out drops a bonus item

    int carrier() const { return counter / 2; }
    void setCarrier(int player) { counter = static_cast<int16_t>(player * 2); }
};

/** A memory word of the original the model cannot read (left by an earlier use of the slot). */
struct Leftover {
    uint16_t word = 0;
};

/** An enemy (level list C): flyer (pterodactyl), walker (triceratops), blower, tree. */
struct Enemy {
    const EnemyKind* kind = nullptr;
    const EnemyState* state = nullptr;
    int16_t pad = 0;                  // walker, tree
    Fixed x, y;
    // the flyer's flight animation, the tree's next bonus item: one word in the original (2cd5)
    std::variant<Leftover, const Animation*, DropCursor> table;
    int16_t startDelay = 0;           // flyer: frames before its first flight
    Fixed vx;
    int16_t timer = 0;                // countdowns; the falling flyer's y speed
    int16_t anim = 0, animDelay = 0;
    Sprite sprite = NO_SPRITE;
    // the walker's facing (0 = left, 2 = right), the flyer's target player: one word in the original (2d57)
    int16_t facing = 0;
};

/** A bonus item (12 slots); free while its sprite is NO_SPRITE. */
struct BonusItem {
    const BonusKind* kind = nullptr;
    const BonusState* state = nullptr;
    Sprite sprite = NO_SPRITE;
    Fixed x, y;                 // top left
    int16_t vx = 0;             // x speed (1/32 px per frame) while falling, then the frames it stays (2e5b)
    int16_t vy = 0;             // y speed (1/32 px per frame)

    bool used() const { return sprite != NO_SPRITE; }
};

/** The water: its surface rises or sinks every second frame. */
struct Water {
    Fixed level;                // surface (28fe)
    int16_t row = 0;            // surface in whole pixels (2903)
    uint8_t hold = 0;           // a frame without movement after the row changed (27a2)
    uint8_t toggle = 0;         // every second frame (27ce)
    int16_t surfaceFrame = 0;   // animation of the surface, 2 .. 0 (27a4)
    uint8_t surfaceDelay = 0;   //   and its countdown (27a3)
};

/**
 * The rain of the windy levels: 193 drops falling diagonally down to the water. A drop's position is a pixel index
 * in the 384 px wide background page (y * 384 + x), so a drop blown over the edge runs into the next row like in
 * the original.
 */
struct Rain {
    static constexpr int DROPS = 193;
    std::array<int32_t, DROPS> drops{};
    uint8_t floorRow = 180;     // drops at or below this row start again (2907 = row * 96)
};

/** The palette fade: the play runs while it is on its way up or at the top. */
struct Fade {
    int16_t position = 0;       // 0 black .. 0x100 full (27a8)
    int16_t step = 0;           // per frame; negative fades out to the end of the level (27aa)

    bool fadingOut() const { return step < 0; }

    /** Starts the fade-out that ends the level attempt. */
    void startFadeOut() {
        step = -2;
        position = static_cast<int16_t>(position - 2);
    }
};

struct World {
    // the game
    uint16_t players = 1;       // 1 or 2 (team)
    uint16_t difficulty = 1;    // 0 .. 2
    uint16_t levelNumber = 0;
    const LevelDefinition* level = nullptr;   // follows from players and levelNumber
    uint8_t lives = 0;
    uint8_t multiplier = 0;
    uint32_t score = 0;
    Random random;

    // the level
    int16_t energy = 0;         // shared by the copters; nothing happens at zero (the status line shows it)
    Fade fade;
    bool levelDone = false;     // all passengers delivered
    uint8_t passengersLeft = 0;
    uint8_t wind = 0;           // 0 none, 1 to the left, 2 to the right; rain with it
    Water water;
    Rain rain;
    std::array<Copter, 2> copters;
    std::array<Pad, 10> pads;
    int padCount = 0;
    std::array<Passenger, 16> passengers;
    int passengerCount = 0;
    std::array<Enemy, 5> enemies;
    int enemyCount = 0;
    std::array<BonusItem, 12> bonuses;

    /** The copters in play: both in the team mode. */
    int copterCount() const { return players == 2 ? 2 : 1; }

    /** The first copter landed on the pad; -1 = none. */
    int copterOnPad(int16_t pad) const {
        for (int c = 0; c < copterCount(); c++)
            if (copters[c].landedPad == pad) return c;
        return -1;
    }

    /** -1 for wind to the left, 1 to the right (for a level with wind). */
    int windDirection() const { return (wind - 1) * 2 - 1; }
};

}  // namespace ugh
