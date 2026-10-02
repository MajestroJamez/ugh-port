// UGH! game logic: level start, the play frame, copter physics, water, rain, keyboard, random numbers.
// Ported from the Kotlin port (core/src/main/kotlin/ugh/core/game), which is verified instruction by instruction
// against the original; each function names both.
#include "sim.hpp"

#include <algorithm>

namespace ugh {

namespace {

constexpr int ROW_BYTES_OFF = 0x00c3;    // DGROUP: bytes per VGA row and plane (0x60)
constexpr int CRASH_LIMITS = 0x262e;     // DGROUP: max impact speed per difficulty
constexpr int LEVEL_COUNTS = 0x2624;     // DGROUP: levels of 1 player / team
constexpr int LEVEL_TABLES = 0x2909;     // DGROUP: level record tables of 1 player / team
constexpr int COPTER_SPRITES = 0x0cfa;   // DGROUP: first / end sprite of the rotor animation per player
constexpr int BACK_PAGE = 0xa988;        // CS:4610 - background page (collision mask) in VGA memory (Mode X 192)
constexpr int PAGE_SIZE = 0x4800;        // CS:4602
constexpr int PROBE_OFFSETS[10] = {0, 3, 5, 0x720, 0x723, 0x725, 0x240, 0x245, 0x480, 0x485};

/** 113b:3d4d - Game.times3quarter: y - (y >> 2), Amiga rows to PC rows. */
int times3quarter(int y) { return w16(y - (s16(y) >> 2)); }

}  // namespace

// ---------------------------------------------------------------- random numbers

/** 113b:4f09 - Draw.kt random(): 0 .. range-1, state CS:4ef7..4efe updated by an add-with-carry chain. */
int Sim::random(int range) {
    auto& r = state.rng;
    int ax = range + r[0];
    int carry = ax >> 16;
    ax = w16(ax);
    int dx = r[1] + ax + carry;
    carry = dx >> 16; dx = w16(dx); r[1] = dx;
    int cx = r[2] + dx + carry;
    carry = cx >> 16; cx = w16(cx); r[2] = cx;
    int dx2 = r[3] + cx + carry;
    carry = dx2 >> 16; dx2 = w16(dx2); r[3] = dx2;
    ax = w16(ax + dx2 + carry);
    r[0] = ax;
    return w16(static_cast<int>((static_cast<int64_t>(ax) * range) >> 16));
}

// ---------------------------------------------------------------- keyboard

/**
 * 113b:4567 - Host.kt keyboardInterrupt: matches scancode sequences (0xE0 prefixes) against the key table
 * DGROUP:281c (6 bytes: sequence ending with 0xff, slot, value, match flag) and sets the key byte 278c + slot.
 * Both players use the keyboard (DGROUP:263f / 2641 = -1).
 */
void Sim::key(int scancode) {
    State& s = state;
    int al = b8(scancode);
    s.lastScancode = al;
    auto& t = s.keyMatch;  // copy of DGROUP:281c ..
    auto at = [&](int off) -> uint8_t& { return t[off - 0x281c]; };
    bool partial = false, matched = false;
    int pos = s.keyPosition;
    for (int si = 0; at(0x281c + si) != 0xff; si += 6) {
        if (at(0x2821 + si) == 0xff) continue;
        if (al != at(0x281c + si + pos)) {
            at(0x2821 + si) = 0xff;
        } else if (at(0x281d + si + pos) != 0xff) {
            partial = true;
        } else {
            int slot = at(0x281f + si);
            s.keys[slot] = at(0x2820 + si);
            matched = true;
            break;
        }
    }
    if (matched || !partial) {
        s.keyPosition = 0;
        for (int si = 0; at(0x281c + si) != 0xff; si += 6) at(0x2821 + si) = 0;
    } else {
        s.keyPosition = b8(s.keyPosition + 1);
    }
}

// ---------------------------------------------------------------- game and level flow

/** 113b:3961 - GameFlow.kt newGame. */
void Sim::newGame() {
    state.lives = 3;
    state.multiplier = 1;
    state.scoreLo = 0;
    state.scoreHi = 0;
}

/** 113b:0fa7 .. (GameFlow.kt playGame after playLevel): next level, or one life less. */
int Sim::levelEnd() {
    State& s = state;
    if (s.levelDone & 0x80) {
        s.level = w16(s.level + 1);
        if (s.level >= data_.u16(LEVEL_COUNTS + 2 * s.players - 2)) return 2;
    } else {
        s.lives = b8(s.lives - 1);
        if (s.lives == 0 || (s.lives & 0x80)) return 1;
        s.multiplier = 1;
    }
    return 0;
}

/** DGROUP offset of the record of the current level (113b:3976). */
int Sim::levelRecord() const {
    int table = data_.u16(LEVEL_TABLES + ((state.players - 1) << 1));
    return data_.u16(table + (state.level << 1));
}

/**
 * 113b:3d66 (Level.kt levelSetup) up to the caption's first retrace wait: energy, fade direction, the level
 * load, and the caption (113b:0664, levelCaption) moving the water row out of the way while it is shown.
 */
void Sim::levelStart() {
    // 0xc4 words from the far pointer DGROUP:b7 = DGROUP:2648 .. 27cf: level map, key states, water counters,
    // fade, status line, level done flag
    State& s = state;
    s.keys.fill(0);
    s.copters[0].keysKnown = s.copters[1].keysKnown = true;
    s.waterHold = s.waterAnimDelay = s.waterAnim = s.waterToggle = 0;
    s.fade = s.fadeStep = 0;
    s.levelDone = 0;
    state.energy = 0x5a3b;
    state.fadeStep = 2;
    loadLevel();
    state.savedWaterRow = state.waterRow;
    state.waterRow = 0xaf;
}

/** 113b:3976 - Level.kt loadLevel (the passengers and objects of lists B and C are not ported yet). */
void Sim::loadLevel() {
    State& s = state;
    int rec = levelRecord();
    auto r16 = [&](int off) { return data_.u16(rec + off); };
    s.passengersLeft = data_.u8(rec + 0x08);
    s.wind = data_.u8(rec + 0x0c);
    int waterRaw = r16(0x15);

    // players: 28f6 / 28f8 and 28fa / 28fc = record +0d .. +14
    for (int p = 0; p < 2; p++) {
        Copter& c = s.copters[p];
        int ax = w16(r16(0x0d + 4 * p) - 0x200);
        c.xf = ax; c.x = w16(s16(ax) >> 5);
        ax = w16(times3quarter(r16(0x0f + 4 * p)) - 0x140);
        c.yf = ax; c.y = w16(s16(ax) >> 5);
        c.landed = 0xffff;
        c.animCounter = 0xffff;
        c.carrying = 0; c.targetPad = 0; c.fare = 0;
        c.vx = 0; c.vy = 0;
    }
    s.copters[0].sprite = 0xda;
    s.copters[1].sprite = 0xe0;

    int water = times3quarter(waterRaw);
    if (water > 0x17e0) water = 0x17e0;  // unsigned compare
    s.waterYf = water;
    s.waterRow = w16(s16(water) >> 5);

    // pads (list A, 8 words, the loader copies 7)
    int si = r16(0x02);
    for (int i = 0; i < PADS; i++) {
        Pad& pad = s.pads[i];
        int w0 = data_.u16(si); si += 2;
        pad = Pad{};
        pad.left = w0;
        if (s16(w0) < 0) break;
        pad.right = data_.u16(si); si += 2;
        pad.y = times3quarter(data_.u16(si)); si += 2;
        pad.doorX = data_.u16(si); si += 2;
        pad.waitX = data_.u16(si); si += 2;
        pad.standX = data_.u16(si); si += 2;
        pad.number = data_.u16(si); si += 2;
        si += 2;
        pad.waiting = 0xffff;
    }

    // rain (level flag +0c): the planes are only set by the spawn
    s.rainOffset.fill(0);
    if (s.wind != 0) {
        for (int bx = 0x180; bx >= 0; bx -= 2) spawnRaindrop(bx);
        for (int i = 0; i < 0x241; i++) moveRain();
    }
    s.rainKnown = s.wind != 0;
}

// ---------------------------------------------------------------- rain

/** 113b:3c35 - Draw.kt spawnRaindrop: restarts drop bx/2 at a random position on the top or side edge. */
void Sim::spawnRaindrop(int bx) {
    State& s = state;
    int ax = random(w16(0x140 + s.waterRow));
    int cx;
    if (ax < 0x140) {
        cx = ax;
        ax = 0;
    } else {
        ax = w16(ax - 0x140);
        cx = b8(s.wind) == 1 ? 0x13f : 0;
    }
    ax = w16((ax & 0xff) * data_.u8(ROW_BYTES_OFF));
    ax = w16(ax + (cx >> 2));
    s.rainOffset[bx >> 1] = ax;
    s.rainPlane[bx >> 1] = cx & 3;
}

/** 113b:3c78 - Draw.kt moveRain: every drop down to the water row moves diagonally; the ones past it restart. */
void Sim::moveRain() {
    State& s = state;
    int bx = w16((s.waterRow + 1) << 1);
    int bp = w16(((b8(s.wind) - 1) << 1) - 1);
    while (true) {
        int ax = s.rainOffset[bx >> 1];
        ax = w16(ax + 0xc0);
        if ((bx & 2) == 0) ax = w16(ax + 0x60);
        int cx = w16(s.rainPlane[bx >> 1] + bp + bp);
        if ((bx & 2) == 0) cx = w16(cx + bp);
        if (cx & 0xfc) ax = w16(ax + bp);
        cx &= 3;
        if (ax >= s.rainFloor) spawnRaindrop(bx);
        else { s.rainOffset[bx >> 1] = ax; s.rainPlane[bx >> 1] = cx; }
        bx = w16(bx - 2);
        if (s16(bx) < 0) break;
    }
}

// ---------------------------------------------------------------- water

/** 113b:2d1c - Draw.kt updateWater (logic part): the water level moves every second frame. */
void Sim::updateWater() {
    State& s = state;
    if (s.waterHold != 0) { s.waterHold = 0; return; }
    int previous = s.waterRow;
    int ax = s.waterYf;
    s.waterToggle = s.waterToggle ^ 1;
    if (s.waterToggle == 0) {
        ax = w16(ax + data_.u16(levelRecord() + 0x17));
        if (s16(ax) < 0) ax = 0;
    }
    s.waterYf = ax;
    s.waterRow = w16(s16(ax) >> 5);
    if (s.waterRow == previous) return;
    s.waterHold = 0xff;  // the next frame redraws instead of moving
}

/** 113b:2db9 - Draw.kt drawWaterSurface (logic part): animation of the surface, where the rain restarts. */
void Sim::drawWaterSurface() {
    State& s = state;
    int si = s.waterAnim;
    s.waterAnimDelay = b8(s.waterAnimDelay - 1);
    if (s.waterAnimDelay & 0x80) {
        s.waterAnimDelay = 6;
        si = w16(si - 1);
        if (s16(si) < 0) si = 2;
        s.waterAnim = si;
    }
    s.rainFloor = w16(b8(s.waterRow) * data_.u8(ROW_BYTES_OFF));
}

// ---------------------------------------------------------------- copter

/**
 * 113b:1457 - Game.kt probe: ORs 10 bytes of one plane of the background page around the copter and tests
 * colour bit 7. The page is the collision mask of the level; outside it the original reads other VGA memory,
 * which never held a solid pixel there in the golden replays (re/notes/phase2-data.md).
 */
bool Sim::probe(int si, int plane) const {
    auto it = data_.masks.find(levelRecord());
    if (it == data_.masks.end()) return false;
    const auto& mask = it->second;
    for (int k : PROBE_OFFSETS) {
        int rel = w16(si + k) - BACK_PAGE;
        if (rel < 0 || rel >= PAGE_SIZE) continue;
        int row = rel / 0x60, x = (rel % 0x60) * 4 + plane;
        if (mask[row * MASK_ROW_BYTES + x / 8] & (0x80 >> (x & 7))) return true;
    }
    return false;
}

/** 113b:1095 - Game.kt copterUpdate: physics of the copter of player p. */
void Sim::copterUpdate(int p) {
    State& s = state;
    Copter& c = s.copters[p];
    s.energy = w16(s.energy - 1);
    c.impact = 0;
    c.effort = 0;
    // CH: 1 = below the water line, 0 = exactly on it, negative = above
    int depth = s16(s16(c.y) - s16(s.waterRow) + 0x12);
    int ch = depth == 0 ? 0 : depth < 0 ? static_cast<int8_t>(depth >> 8) : 1;
    if (s16(c.landed) < 0) moveHorizontally(p, ch);
    moveVertically(p, ch);
}

/** Game.kt moveHorizontally: wind, left / right, movement with the collision probe, bouncing off walls. */
void Sim::moveHorizontally(int p, int ch) {
    State& s = state;
    Copter& c = s.copters[p];
    int bx = 2 * p;
    if (ch < 0 && b8(s.wind) != 0) {
        int ax = s16(w16((((b8(s.wind) - 1) << 1) - 1) << 5));
        c.vx = w16(c.vx + ax);
        ax >>= 1;
        if (ax < 0) ax = s16(-ax);
        c.vy = w16(c.vy + ax);
    }
    int ax = s16(c.vx);
    if (key16(KEY_AREA + 0x08 + bx) != 0) { ax = s16(ax - 0x3f); c.effort = w16(c.effort + 0x3f); }
    else if (key16(KEY_AREA + 0x0c + bx) != 0) { ax = s16(ax + 0x3f); c.effort = w16(c.effort + 0x3f); }
    ax = std::clamp(ax, -0x1800, 0x1800);
    c.vx = w16(ax);

    ax = s16((ax >> 6) + s16(c.xf));
    if (ax < -0x200) { ax = -0x200; c.vx = 0; }
    else if (ax > 0x2600) { ax = 0x2600; c.vx = 0; }
    int di = ax;

    int row = s16(s16(c.y) * data_.u8(ROW_BYTES_OFF));
    int si = w16(BACK_PAGE + s16(row + ((s16(c.x) + 5) >> 2)));
    int cl = (s16(c.x) + 0x25) & 3;

    int bp = di;
    if ((di >> 5) != s16(c.x)) {
        bp = s16(c.xf);
        bool hit = false;
        if (di <= bp) {
            // moving left - original: the probe position is not advanced inside the loop
            cl--;
            if (cl < 0) { cl &= 3; si = w16(si - 1); }
            while (true) {
                if (probe(si, cl)) { bp = s16((bp & 0xffe0) + 0x20); hit = true; break; }
                bp = s16(bp - 0x20);
                if (bp <= di) break;
            }
        } else {
            while (true) {
                cl++;
                if (cl >= 4) { cl &= 3; si = w16(si + 1); }
                if (probe(si, cl)) { bp = s16(bp & 0xffe0); hit = true; break; }
                bp = s16(bp + 0x20);
                if (bp >= di) break;
            }
        }
        if (hit) {
            int v = s16(-s16(c.vx)) >> 1;
            c.vx = w16(v);
            v = s16(v << 1);
            if (v < 0) v = s16(-v);
            c.impact = w16(v);
        } else {
            bp = di;
        }
    }
    c.xf = w16(bp);
    c.x = w16(bp >> 5);
}

/** Game.kt moveVertically: buoyancy, gravity, diving, pedalling, movement with the probe, crash. */
void Sim::moveVertically(int p, int ch) {
    State& s = state;
    Copter& c = s.copters[p];
    int bx = 2 * p;
    int ax = s16(c.vy);
    bool checkUp = false;
    if (ch > 0) {  // in the water: buoyancy
        s.energy = w16(s.energy - 2);
        if (ax > 0) ax = s16(ax - 0xc1); else { ax = s16(ax - 0x15); c.landed = 0xffff; }
    } else if (ch < 0 && s16(c.landed) < 0) {  // flying above the water: gravity, diving
        ax = s16(ax + 0x1b);
        s.energy = w16(s.energy - 2);
        if (key16(KEY_AREA + 0x04 + bx) != 0) { ax = s16(ax + 0x46); c.effort = w16(c.effort + 0x5a); } else checkUp = true;
    } else {
        checkUp = true;
    }
    if (checkUp && key16(KEY_AREA + 0x00 + bx) != 0) {  // pedalling
        s.energy = w16(s.energy - 3);
        ax = s16(ax - 0x46);
        c.effort = w16(c.effort + 0x5a);
        c.landed = 0xffff;
    }
    ax = std::clamp(ax, -0x1800, 0x1800);
    c.vy = w16(ax);

    ax = s16((ax >> 6) + s16(c.yf));
    if (ax < -0x260) { ax = -0x260; c.vy = 0; }
    else if (ax > 0x17e0) { ax = 0x17e0; c.vy = 0; }
    if (ch > 0) {
        int dx = s16((ax >> 5) - s16(s.waterRow) + 0x12);
        if (dx <= 0) { c.vy = 0; ax = s16((s16(s.waterRow) << 5) - 0x240); }
    }
    int di = ax;

    int plane = (s16(c.x) + 0x25) & 3;
    int row = s16(s16(c.y) * data_.u8(ROW_BYTES_OFF));
    int si = w16(BACK_PAGE + s16(row + ((s16(c.x) + 5) >> 2)));

    int bp = di;
    if ((di >> 5) != s16(c.y)) {
        bp = s16(c.yf);
        bool hit = false;
        if (di <= bp) {
            // moving up - original: the probe position is not advanced inside the loop
            si = w16(si - 0x60);
            while (true) {
                if (probe(si, plane)) { bp = s16((bp & 0xffe0) + 0x20); hit = true; break; }
                bp = s16(bp - 0x20);
                if (bp <= di) break;
            }
        } else {
            while (true) {
                si = w16(si + 0x60);
                if (probe(si, plane)) { bp = s16(bp & 0xffe0); hit = true; break; }
                bp = s16(bp + 0x20);
                if (bp >= di) break;
            }
        }
        if (hit) bounceVertically(p, bp); else bp = di;
    }
    c.yf = w16(bp);
    c.y = w16(bp >> 5);

    // crash: impact too hard for the difficulty
    if (s16(c.impact) >= data_.s16w(CRASH_LIMITS + 2 * s16(s.difficulty)) && s16(s.fadeStep) >= 0) {
        s.fadeStep = 0xfffe;
        s.fade = w16(s.fade - 2);
    }
}

/** Game.kt bounceVertically: bounce, impact, soft touch-down on the pad under the copter. */
void Sim::bounceVertically(int p, int bp) {
    State& s = state;
    Copter& c = s.copters[p];
    int ax = s16(-s16(c.vy)) >> 1;
    c.vy = w16(ax);
    ax = s16(ax << 1);
    if (ax < 0) ax = s16(-ax);
    if (ax > s16(c.impact)) c.impact = w16(ax);
    if (s16(c.vy) >= 0) return;  // hit a ceiling
    if (s16(c.impact) >= data_.s16w(CRASH_LIMITS + 2 * s16(s.difficulty))) return;

    int padY = s16((bp >> 5) + 0x14);
    for (int i = 0; i < PADS && s.pads[i].left != UNKNOWN && s16(s.pads[i].left) >= 0; i++) {
        const Pad& pad = s.pads[i];
        if (s16(pad.y) != padY) continue;
        int cx = s16(s16(c.x) + 0x10);
        if (cx >= s16(pad.left) && cx <= s16(pad.right)) {
            c.landed = i;
            c.vx = 0;
            c.vy = 0;
            return;
        }
    }
}

/** 113b:418d - Draw.kt drawCopter (logic part): rotor animation, its speed from the effort. */
void Sim::drawCopter(int p) {
    Copter& c = state.copters[p];
    int ax = s16(c.effort);
    if (ax > 0x5a) ax = 0x5a;
    c.animCounter = w16(c.animCounter - ax);
    ax = s16(c.animCounter);
    if (ax < 0) {
        c.animCounter = w16(ax + 0xc8);
        ax = s16(c.sprite) + 1;
        if (ax >= data_.s16w(COPTER_SPRITES + 2 + 2 * p)) ax = data_.s16w(COPTER_SPRITES + 2 * p);
        c.sprite = w16(ax);
    }
}

// ---------------------------------------------------------------- the play frame

/**
 * One frame of the level play: 113b:0c7d .. 0fa4 (GameFlow.kt playLevel, Frame.kt frameBody / frameAfterKeys)
 * after the retrace wait that starts it. Passengers, objects and bonus items are not ported yet.
 */
void Sim::playFrame() {
    State& s = state;
    // the fade position advances right after the retrace wait (playLevel)
    if (s16(s.fade) <= 0x100) s.fade = w16(s.fade + s16(s.fadeStep));

    updateWater();
    int key = s.lastScancode;  // 113b:0fe8 frameKeys: the last scancode of the keyboard handler
    if (key == 0x19) problems.push_back("pause (P) is not supported");
    if (key == 0x01) {  // Esc
        s.lives = 0;
        if (s16(s.fadeStep) >= 0) { s.fadeStep = 0xfffe; s.fade = w16(s.fade - 2); }
    }
    if (s16(s.fade) > 0xc0 || s16(s.fadeStep) <= 0) {
        int bx = 0;
        do { copterUpdate(bx >> 1); bx += 2; } while (bx == s.players);
    }
    // passengers (113b:1486), objects (2363), bonus items (2b7f): steps 6 and 7 of re/notes/plan.md

    int bx = 0;
    do { drawCopter(bx >> 1); bx += 2; } while (bx != 2 * s.players);
    if (b8(s.wind) != 0) moveRain();
    drawWaterSurface();
}

}  // namespace ugh
