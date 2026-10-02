// UGH! game logic: game and level flow, the play frame, copter physics, water, rain, keyboard, random numbers.
#include <algorithm>

#include "sim.hpp"

namespace ugh {

namespace {

constexpr int BACK_PAGE = 0xa988;   // CS:4610 - background page (collision mask) in VGA memory (Mode X 192)
constexpr int PAGE_SIZE = 0x4800;   // CS:4602
constexpr int PROBE_OFFSETS[10] = {0, 3, 5, 0x720, 0x723, 0x725, 0x240, 0x245, 0x480, 0x485};

}  // namespace

Sim::Sim(Data data) : data_(std::move(data)) { reset(); }

/** 113b:3d4d - Game.kt times3quarter: y - (y >> 2), Amiga rows to PC rows (the quarter stays in 2648). */
int Sim::times3quarter(int y) {
    int quarter = s16(y) >> 2;
    setD(0x2648, quarter);
    return w16(y - quarter);
}

/** 113b:3d5a - Game.kt times3half: x + (x >> 1). */
int Sim::times3half(int x) {
    int half = s16(x) >> 1;
    setD(0x2648, half);
    return w16(x + half);
}

// ---------------------------------------------------------------- random numbers

/** 113b:4f09 - Draw.kt random(): 0 .. range-1, state CS:4ef7..4efe updated by an add-with-carry chain. */
int Sim::random(int range) {
    auto& r = rng_;
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
 * DGROUP:281c and sets the key states DGROUP:278c + slot. Both players use the keyboard (263f / 2641 = -1).
 */
void Sim::key(int scancode) {
    int al = b8(scancode);
    lastScancode_ = al;
    bool partial = false, matched = false;
    int pos = d8(0x2647);
    for (int si = 0; d8(0x281c + si) != 0xff; si += 6) {
        if (d8(0x2821 + si) == 0xff) continue;
        if (al != d8(0x281c + si + pos)) {
            setD8(0x2821 + si, 0xff);
        } else if (d8(0x281d + si + pos) != 0xff) {
            partial = true;
        } else {
            setD8(0x278c + d8(0x281f + si), d8(0x2820 + si));
            matched = true;
            break;
        }
    }
    if (matched || !partial) {
        setD8(0x2647, 0);
        for (int si = 0; d8(0x281c + si) != 0xff; si += 6) setD8(0x2821 + si, 0);
    } else {
        setD8(0x2647, d8(0x2647) + 1);
    }
}

// ---------------------------------------------------------------- game and level flow

/** 113b:3961 - GameFlow.kt newGame. */
void Sim::newGame() {
    setD8(0x263c, 3);
    setD8(0x263d, 1);
    setD(0x261e, 0);
    setD(0x2620, 0);
}

/** 113b:0fa7 .. (GameFlow.kt playGame after playLevel): next level, or one life less and multiplier 1. */
int Sim::levelEnd() {
    if (d8(0x27cf) & 0x80) {
        int next = u(0x261c) + 1;
        setD(0x261c, next);
        if (next >= u(0x2624 + u(PLAYERS2) - 2)) return 2;
    } else {
        int lives = b8(d8(0x263c) - 1);
        setD8(0x263c, lives);
        if (lives == 0 || (lives & 0x80)) return 1;
        setD8(0x263d, 1);
    }
    return 0;
}

/** DGROUP offset of the record of the current level (113b:3976). */
int Sim::levelRecord() const {
    int table = u(0x2909 + ((u(PLAYERS) - 1) << 1));
    return u(table + (u(0x261c) << 1));
}

/**
 * 113b:3d66 (Level.kt levelSetup) up to the caption's first retrace wait: the state, the level load, and the
 * caption (113b:0664, levelCaption) moving the water row out of the way while it is shown.
 */
void Sim::levelStart() {
    prepare();
    // 0xc4 words from the far pointer DGROUP:b7 = DGROUP:2648 .. 27cf: level map, key states, water counters,
    // fade, status line, level done flag
    for (int i = 0; i < 0xc4; i++) setD(0x2648 + 2 * i, 0);
    setD(V_ENERGY, 0x5a3b);
    setD(0x27c8, 0xffff);
    setD(V_FADE_STEP, 2);
    // the status bar redraws everything (113b:3d66 .. 3dd8)
    auto offset = [](int v) { int al = b8(v + 0xb); if (al >= 0x64) al -= 0x64; return al; };
    setD8(0x27cb, offset(d8(0x263c)));
    setD8(0x27ca, offset(d8(0x263d)));
    setD8(0x27cc, 0x63);
    setD(0x27c6, 0x270f);
    long long score = ((static_cast<long long>(u(0x2620)) << 16) | u(0x261e)) + 0x1b207;
    if (score > 0xf4240) score -= 0xf4240;
    setD(0x27c2, static_cast<int>(score & 0xffff));
    setD(0x27c4, static_cast<int>((score >> 16) & 0xffff));
    setD(0x27ae, 0xc2);
    if (u(PLAYERS) == 2) {
        setD(0x27b0, 0x21); setD(0x27b2, 0x72); setD(0x27b4, 0xa9); setD(0x27b8, 0xc7); setD(0x27b6, 0xd7);
        setD(0x27ba, 0xe8); setD(0x27bc, 0x131); setD(0x27be, 0x102); setD(0x27c0, 0x112);
    } else {
        setD(0x27b0, 0x21); setD(0x27b2, 0x76); setD(0x27b4, 0xb1); setD(0x27b8, 0xd0); setD(0x27b6, 0xe0);
        setD(0x27ba, 0xf4); setD(0x27bc, 0x122);
    }
    loadLevel();
    savedWaterRow_ = u(V_WATER_ROW);
    setD(V_WATER_ROW, 0xaf);
}

/** 113b:3976 - Level.kt loadLevel: level record, players, water, pads (A), passengers (B), objects (C), rain. */
void Sim::loadLevel() {
    int bx = u(0x2909 + ((u(PLAYERS) - 1) << 1));
    bx = u(bx + (u(0x261c) << 1));
    for (int si = 0x18; si >= 0; si -= 2) setD(0x28e9 + si, u(bx + si));
    // the map (u(record)) is depacked and copied to 264c for the renderer

    auto startPos = [&](int fx, int fy, int pl) {
        int ax = w16(u(fx) - 0x200);
        setD(P_XF + pl, ax); setD(P_X + pl, s16(ax) >> 5); setD(0x27e0 + pl, s16(ax) >> 5);
        ax = w16(times3quarter(u(fy)) - 0x140);
        setD(P_YF + pl, ax); setD(P_Y + pl, s16(ax) >> 5); setD(0x27e4 + pl, s16(ax) >> 5);
    };
    startPos(0x28f6, 0x28f8, 0);
    startPos(0x28fa, 0x28fc, 2);
    for (int o : {0x27f8, 0x27fa, 0x27f0, 0x27f2}) setD(o, 0xffff);
    setD(0x27e8, 0xda); setD(0x27ea, 0xe0);
    setD(0x2800, 0xffff); setD(0x2802, 0xffff);
    setD8(0x27cd, 0xff);
    for (int o : {0x27fc, 0x27fe, 0x2804, 0x2806, 0x2808, 0x280a, 0x2810, 0x2812, 0x2814, 0x2816}) setD(o, 0);
    int water = times3quarter(u(0x28fe));
    if (water > 0x17e0) water = 0x17e0;  // unsigned compare
    setD(0x28fe, water);
    setD(V_WATER_ROW, s16(water) >> 5);
    setD(0x2905, s16(water) >> 5);

    // pads (list A)
    int si = u(0x28eb);
    int di = 0;
    while (true) {
        int w0 = u(si); si += 2;
        setD(PAD_LEFT + di, w0);
        if (s16(w0) < 0) break;
        setD(PAD_RIGHT + di, u(si)); si += 2;
        setD(PAD_Y + di, times3quarter(u(si))); si += 2;
        setD(0x2949 + di, u(si)); si += 2;
        setD(0x295d + di, u(si)); si += 2;
        setD(0x2971 + di, u(si)); si += 2;
        setD(0x2985 + di, u(si)); si += 2;
        si += 2;
        setD(0x2999 + di, 0xffff);
        di += 2;
    }

    // passengers (list B)
    si = u(0x28ed);
    di = 0;
    while (true) {
        int type = u(si); si += 2;
        setD(0x29ad + di, type);
        if (type == 0xffff) break;
        setD(0x29cd + di, u(si)); si += 2;
        setD(0x29ed + di, u(si)); si += 2;
        for (int o : {0x2bed, 0x2c0d, 0x2c2d, 0x2c4d}) setD(o + di, 0xffff);
        if (type == 0x78dc) {
            setD(0x2a0d + di, u(type + 0x10));
            int pad = u(0x29cd + di) << 1;
            setD(0x2aed + di, w16((u(0x2971 + pad) - 8) << 5));
            setD(0x2b0d + di, w16((u(0x2935 + pad) - 0xb) << 5));
        } else {
            setD(0x2a0d + di, u(type + 8));
        }
        di += 2;
    }

    // objects (list C)
    si = u(0x28ef);
    di = 0;
    while (true) {
        int type = u(si); si += 2;
        setD(0x2cad + di, type);
        if (type == 0xffff) break;
        if (type == 0x76e4) {
            int pad = u(si); si += 2;
            setD(0x2cb7 + di, pad);
            setD(0x2cc1 + di, w16((u(0x2971 + (pad << 1)) - 0x10) << 5));
            setD(0x2ccb + di, w16((u(0x2935 + (pad << 1)) - 0x18) << 5));
            setD(0x2cd5 + di, u(si)); si += 2;
            setD(0x2cf3 + di, 0x2a87);
        } else if (type == 0x7630) {
            si += 2;
            setD(0x2cdf + di, times3half(u(si))); si += 2;
            setD(0x2ce9 + di, u(si)); si += 2;
            setD(0x2d57 + di, 1);
            setD(0x2cf3 + di, 0x2379);
        } else if (type == 0x76a8) {
            setD(0x2cc1 + di, w16(u(si) << 5)); si += 2;
            setD(0x2ccb + di, w16(times3quarter(w16(u(si) << 5)) - 0x2c0)); si += 2;
            si += 4;
            setD(0x2cf3 + di, 0x295b);
        } else {
            int pad = u(si); si += 2;
            setD(0x2cb7 + di, pad);
            setD(0x2cc1 + di, w16((u(0x2971 + (pad << 1)) - 0x10) << 5));
            setD(0x2ccb + di, w16((u(0x2935 + (pad << 1)) - 0x16) << 5));
            si += 2;
            int speed = u(si); si += 2;
            setD(0x2ce9 + di, w16(-(-(s16(speed) >> 2) + speed)));
            setD(0x2d57 + di, 0);
            setD(0x2cf3 + di, 0x25b1);
        }
        di += 2;
    }

    // rain (level flag +0c): the planes (318f) are only set by the spawn
    for (int b = 0x180; b >= 0; b -= 2) { setD(0x2e8b + b, 0); setD(0x300d + b, 0); }
    if (d8(V_WIND) != 0) {
        for (int b = 0x180; b >= 0; b -= 2) spawnRaindrop(b);
        for (int i = 0; i < 0x241; i++) moveRain();
    }
    rainKnown_ = d8(V_WIND) != 0;
}

// ---------------------------------------------------------------- rain

/** 113b:3c35 - Draw.kt spawnRaindrop: restarts raindrop bx at a random position on the top or side edge. */
void Sim::spawnRaindrop(int bx) {
    int ax = random(w16(0x140 + u(V_WATER_ROW)));
    int cx;
    if (ax < 0x140) {
        cx = ax;
        ax = 0;
    } else {
        ax = w16(ax - 0x140);
        cx = d8(V_WIND) == 1 ? 0x13f : 0;
    }
    ax = w16((ax & 0xff) * d8(V_ROW_BYTES));
    ax = w16(ax + (cx >> 2));
    setD(0x2e8b + bx, ax);
    setD(0x318f + bx, cx & 3);
}

/** 113b:3c78 - Draw.kt moveRain: the drops down to the water row move diagonally; the ones past it restart. */
void Sim::moveRain() {
    int bx = w16((u(V_WATER_ROW) + 1) << 1);
    int bp = w16(((d8(V_WIND) - 1) << 1) - 1);
    while (true) {
        int ax = u(0x2e8b + bx);
        setD(0x300d + bx, ax);
        ax = w16(ax + 0xc0);
        if ((bx & 2) == 0) ax = w16(ax + 0x60);
        int cx = w16(u(0x318f + bx) + bp + bp);
        if ((bx & 2) == 0) cx = w16(cx + bp);
        if (cx & 0xfc) ax = w16(ax + bp);
        cx &= 3;
        if (ax >= u(0x2907)) spawnRaindrop(bx);
        else { setD(0x2e8b + bx, ax); setD(0x318f + bx, cx); }
        bx = w16(bx - 2);
        if (s16(bx) < 0) break;
    }
}

// ---------------------------------------------------------------- water

/** 113b:2d1c - Draw.kt updateWater (without the drawing): the water level moves every second frame. */
void Sim::updateWater() {
    if (d8(0x27a2) != 0) { setD8(0x27a2, 0); return; }
    setD(0x2905, u(V_WATER_ROW));
    int ax = u(0x28fe);
    int toggle = d8(0x27ce) ^ 1;
    setD8(0x27ce, toggle);
    if (toggle == 0) {
        ax = w16(ax + u(0x2900));
        if (s16(ax) < 0) ax = 0;
    }
    setD(0x28fe, ax);
    ax = w16(s16(ax) >> 5);
    setD(V_WATER_ROW, ax);
    if (ax == u(0x2905)) return;
    setD8(0x27a2, d8(0x27a2) - 1);  // the next frame redraws instead of moving
}

/** 113b:2db9 - Draw.kt drawWaterSurface (without the drawing): surface animation, where the rain restarts. */
void Sim::drawWaterSurface() {
    int si = u(0x27a4);
    int delay = b8(d8(0x27a3) - 1);
    setD8(0x27a3, delay);
    if (delay & 0x80) {
        setD8(0x27a3, 6);
        si = w16(si - 1);
        if (s16(si) < 0) si = 2;
        setD(0x27a4, si);
    }
    setD(0x2907, b8(u(V_WATER_ROW)) * d8(V_ROW_BYTES));
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

/** 113b:1095 - Game.kt copterUpdate: physics of the copter of player bx (0 or 2). */
void Sim::copterUpdate(int bx) {
    addD(V_ENERGY, -1);
    setD(P_IMPACT + bx, 0);
    setD(P_EFFORT + bx, 0);
    // CH: 1 = below the water line, 0 = exactly on it, negative = above (high byte of the difference)
    int depth = s16(d(P_Y + bx) - d(V_WATER_ROW) + 0x12);
    int ch = depth == 0 ? 0 : depth < 0 ? static_cast<int8_t>(depth >> 8) : 1;
    if (d(P_LANDED + bx) < 0) moveHorizontally(bx, ch);
    moveVertically(bx, ch);
}

/** Game.kt moveHorizontally: wind, left / right, movement with the collision probe, bouncing off walls. */
void Sim::moveHorizontally(int bx, int ch) {
    if (ch < 0) {  // wind above the water (level flag +0c)
        int wind = d8(V_WIND);
        if (wind != 0) {
            int ax = s16((((wind - 1) << 1) - 1) << 5);
            addD(P_VX + bx, ax);
            ax >>= 1;
            if (ax < 0) ax = s16(-ax);
            addD(P_VY + bx, ax);
        }
    }
    int ax = d(P_VX + bx);
    if (d(KEY_LEFT + bx) != 0) { ax = s16(ax - 0x3f); addD(P_EFFORT + bx, 0x3f); }
    else if (d(KEY_RIGHT + bx) != 0) { ax = s16(ax + 0x3f); addD(P_EFFORT + bx, 0x3f); }
    ax = std::clamp(ax, -0x1800, 0x1800);
    setD(P_VX + bx, ax);

    ax = s16((ax >> 6) + d(P_XF + bx));
    if (ax < -0x200) { ax = -0x200; setD(P_VX + bx, 0); }
    else if (ax > 0x2600) { ax = 0x2600; setD(P_VX + bx, 0); }
    int di = ax;

    int row = s16(d(P_Y + bx) * d(V_ROW_BYTES));
    int si = w16(BACK_PAGE + s16(row + ((d(P_X + bx) + 5) >> 2)));
    int cl = (d(P_X + bx) + 0x25) & 3;

    int bp = di;
    if ((di >> 5) != d(P_X + bx)) {
        bp = d(P_XF + bx);
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
            int v = s16(-d(P_VX + bx)) >> 1;
            setD(P_VX + bx, v);
            v = s16(v << 1);
            if (v < 0) v = s16(-v);
            setD(P_IMPACT + bx, v);
        } else {
            bp = di;
        }
    }
    setD(P_XF + bx, bp);
    setD(P_X + bx, bp >> 5);
}

/** Game.kt moveVertically: buoyancy, gravity, diving, pedalling, movement with the probe, crash. */
void Sim::moveVertically(int bx, int ch) {
    int ax = d(P_VY + bx);
    bool checkUp = false;
    if (ch > 0) {  // in the water: buoyancy
        addD(V_ENERGY, -2);
        if (ax > 0) ax = s16(ax - 0xc1); else { ax = s16(ax - 0x15); setD(P_LANDED + bx, -1); }
    } else if (ch < 0 && d(P_LANDED + bx) < 0) {  // flying above the water: gravity, diving
        ax = s16(ax + 0x1b);
        addD(V_ENERGY, -2);
        if (d(KEY_DOWN + bx) != 0) { ax = s16(ax + 0x46); addD(P_EFFORT + bx, 0x5a); } else checkUp = true;
    } else {
        checkUp = true;
    }
    if (checkUp && d(KEY_UP + bx) != 0) {  // pedalling
        addD(V_ENERGY, -3);
        ax = s16(ax - 0x46);
        addD(P_EFFORT + bx, 0x5a);
        setD(P_LANDED + bx, -1);
    }
    ax = std::clamp(ax, -0x1800, 0x1800);
    setD(P_VY + bx, ax);

    ax = s16((ax >> 6) + d(P_YF + bx));
    if (ax < -0x260) { ax = -0x260; setD(P_VY + bx, 0); }
    else if (ax > 0x17e0) { ax = 0x17e0; setD(P_VY + bx, 0); }
    if (ch > 0) {
        int dx = s16((ax >> 5) - d(V_WATER_ROW) + 0x12);
        if (dx <= 0) { setD(P_VY + bx, 0); ax = s16((d(V_WATER_ROW) << 5) - 0x240); }
    }
    int di = ax;

    int plane = (d(P_X + bx) + 0x25) & 3;
    int row = s16(d(P_Y + bx) * d(V_ROW_BYTES));
    int si = w16(BACK_PAGE + s16(row + ((d(P_X + bx) + 5) >> 2)));

    int bp = di;
    if ((di >> 5) != d(P_Y + bx)) {
        bp = d(P_YF + bx);
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
        if (hit) bounceVertically(bx, bp); else bp = di;
    }
    setD(P_YF + bx, bp);
    setD(P_Y + bx, bp >> 5);

    // crash: impact too hard for the difficulty
    if (d(P_IMPACT + bx) >= d(CRASH_LIMITS + 2 * d(V_DIFFICULTY)) && d(V_FADE_STEP) >= 0) {
        setD(V_FADE_STEP, -2);
        addD(V_FADE, -2);
    }
}

/** Game.kt bounceVertically: bounce, impact, soft touch-down on the pad under the copter. */
void Sim::bounceVertically(int bx, int bp) {
    int ax = s16(-d(P_VY + bx)) >> 1;
    setD(P_VY + bx, ax);
    ax = s16(ax << 1);
    if (ax < 0) ax = s16(-ax);
    if (ax > d(P_IMPACT + bx)) setD(P_IMPACT + bx, ax);
    if (d(P_VY + bx) >= 0) return;  // hit a ceiling
    if (d(P_IMPACT + bx) >= d(CRASH_LIMITS + 2 * d(V_DIFFICULTY))) return;

    // soft touch-down: find the pad under the copter
    int padY = s16((bp >> 5) + 0x14);
    for (int si = 0; d(PAD_LEFT + si) >= 0; si += 2) {
        if (d(PAD_Y + si) != padY) continue;
        int cx = s16(d(P_X + bx) + 0x10);
        if (cx >= d(PAD_LEFT + si) && cx <= d(PAD_RIGHT + si)) {
            setD(P_LANDED + bx, si >> 1);
            setD(P_VX + bx, 0);
            setD(P_VY + bx, 0);
            return;
        }
    }
}

/** 113b:418d - Draw.kt drawCopter (without the drawing): rotor animation, its speed from the effort. */
void Sim::drawCopter(int bx) {
    int ax = d(P_EFFORT + bx);
    if (ax > 0x5a) ax = 0x5a;
    addD(0x27f0 + bx, -ax);
    ax = d(0x27f0 + bx);
    if (ax < 0) {
        setD(0x27f0 + bx, ax + 0xc8);
        ax = d(0x27e8 + bx) + 1;
        if (ax >= d(0x0cfc + bx)) ax = d(0x0cfa + bx);
        setD(0x27e8 + bx, ax);
    }
}

/** Frame.kt frameAfterKeys, drawing the passengers (without the drawing): where each one is drawn. */
void Sim::drawPassengers() {
    for (int si = 0; u(0x29ad + si) != 0xffff; si += 2) {
        if (u(0x2bed + si) == 0xffff) continue;
        setD(0x2b4d + si, u(0x2b2d + si));
        setD(0x2b8d + si, u(0x2b6d + si));
        setD(0x2b2d + si, d(0x2aed + si) >> 5);
        setD(0x2b6d + si, d(0x2b0d + si) >> 5);
    }
}

// ---------------------------------------------------------------- the play frame

/**
 * One frame of the level play: 113b:0c7d .. 0fa4 (GameFlow.kt playLevel, Frame.kt frameBody / frameAfterKeys)
 * after the retrace wait that starts it. Objects and bonus items are not ported yet.
 */
void Sim::playFrame() {
    prepare();
    // the fade position advances right after the retrace wait (playLevel)
    int cx = d(V_FADE);
    if (cx <= 0x100) setD(V_FADE, cx + d(V_FADE_STEP));

    updateWater();
    int key = lastScancode_;  // 113b:0fe8 frameKeys: the last scancode of the keyboard handler
    if (key == 0x19) problems.push_back("pause (P) is not supported");
    if (key == 0x01) {  // Esc
        setD8(0x263c, 0);
        if (d(V_FADE_STEP) >= 0) { setD(V_FADE_STEP, 0xfffe); addD(V_FADE, -2); }
    }
    if (d(V_FADE) > 0xc0 || d(V_FADE_STEP) <= 0) {
        int bx = 0;
        do { copterUpdate(bx); bx += 2; } while (bx == u(PLAYERS));
    }
    Regs r;
    passengersUpdate(r);
    // objects (113b:2363) and bonus items (2b7f): step 7 of re/notes/plan.md

    drawPassengers();
    int bx = 0;
    do { drawCopter(bx); bx += 2; } while (bx != u(PLAYERS2));
    if (d8(V_WIND) != 0) moveRain();
    drawWaterSurface();
}

}  // namespace ugh
