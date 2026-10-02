// Passengers (level list B) - 113b:1486 .. 113b:2276, ported from Passengers.kt.
//
// Every passenger is a state machine: DGROUP:2a0d[i] holds the address of its current state handler, the
// handlers come from its descriptor (29ad[i], e.g. 0x7720) and jump to each other with JMP [SI+n], handing
// over registers. Arrays (index = passenger * 2): 29ad descriptor, 29cd start pad, 29ed route, 2a0d state,
// 2a2d counter / carrying player, 2a4d pickup pad, 2a6d target pad, 2a8d / 2aad the same * 2, 2acd timer,
// 2aed / 2b0d x / y (1/32 px), 2b2d / 2b6d x / y (px), 2bad animation index, 2bcd animation delay,
// 2bed sprite, 2c2d bubble, 2c6d vertical speed, 2c8d bonus timer. Pads: 2999 passenger waiting there,
// 2985 number shown. Players: 27fc carried passenger type, 2804 target pad shown, 2808 fare, 280c minimum.
// The sound effects of the original (ADLX blocks 4274, 4632) are left out.
#include "sim.hpp"

#include <algorithm>
#include <cstdio>

namespace ugh {

namespace {

constexpr int P_DESC = 0x29ad, P_ROUTE = 0x29ed, P_STATE = 0x2a0d, P_COUNT = 0x2a2d;
constexpr int P_FROM = 0x2a4d, P_TO = 0x2a6d, P_FROM2 = 0x2a8d, P_TO2 = 0x2aad, P_TIMER = 0x2acd;
constexpr int PX = 0x2aed, PY = 0x2b0d, PPX = 0x2b2d, PPY = 0x2b6d, P_ANIM = 0x2bad, P_DELAY = 0x2bcd;
constexpr int P_SPRITE = 0x2bed, P_BUBBLE = 0x2c2d, PVY = 0x2c6d, P_BONUS = 0x2c8d;
constexpr int PAD_W3 = 0x2949, PAD_W4 = 0x295d, PAD_NUMBER = 0x2985, PAD_WAITING = 0x2999;
constexpr int PL_CARRY = 0x27fc, PL_TARGET = 0x2804, PL_FARE = 0x2808, PL_FARE_MIN = 0x280c;
constexpr int SCORE_LO = 0x261e, SCORE_HI = 0x2620, MULTIPLIER = 0x263d, MULTIPLIER_LIMITS = 0x2628;
constexpr int BONUS_DESCRIPTOR = 0x7a38;  // bonus items dropped for quick deliveries

}  // namespace

/** 113b:1486 - Passengers.kt passengersUpdate: runs the state handler of every passenger. */
void Sim::passengersUpdate(Regs& r) {
    r.bx = 0;
    while (u(P_DESC + r.bx) != 0xffff) {
        passengerState(u(P_STATE + r.bx), r);
        r.bx = w16(r.bx + 2);
    }
}

/** Passengers.kt passengerState: dispatches a state handler address of the original to its port. */
void Sim::passengerState(int addr, Regs& r) {
    switch (addr) {
        case 0x149c: p149cNextStop(r); break;
        case 0x1509: p1509Arriving(r); break;
        case 0x153b: p153bAppear(r); break;
        case 0x1582: p1582Appearing(r); break;
        case 0x15b4: p15b4StartWaiting(r); break;
        case 0x15d7: p15d7Waiting(r); break;
        case 0x16f6: p16f6StartCalling(r); break;
        case 0x172a: p172aCalling(r); break;
        case 0x17e6: p17e6StartImpatient(r); break;
        case 0x180a: p180aImpatient(r); break;
        case 0x18c8: p18c8StartBoarding(r); break;
        case 0x18e6: p18e6Boarding(r); break;
        case 0x19e0: p19e0Board(r, true); break;
        case 0x19fb: p19e0Board(r, false); break;
        case 0x1a42: p1a42Riding(r); break;
        case 0x1a7e: p1a7ePaid(r); break;
        case 0x1b29: p1b29WalkingAway(r); break;
        case 0x1bbe: p1bbeStartEntering(r); break;
        case 0x1bd6: p1bd6Entering(r); break;
        case 0x1c0e: case 0x2195: break;
        case 0x1c0f: p1c0fStartStanding(r); break;
        case 0x1c27: p1c27Standing(r); break;
        case 0x1c48: p1c48Grabbed(r); break;
        case 0x1c6b: p1c6bHanging(r); break;
        case 0x1c81: p1c81Dropped(r); break;
        case 0x1cee: p1ceeFalling(r); break;
        case 0x1da8: p1da8StartSplash(r); break;
        case 0x1dd5: p1dd5Splash(r, false); break;
        case 0x1de2: p1dd5Splash(r, true); break;
        case 0x1e9c: p1e9cStartSinking(r); break;
        case 0x1ec0: p1ec0Sinking(r); break;
        case 0x1f24: p1f24StartSwimming(r); break;
        case 0x1f43: p1f43Swimming(r); break;
        case 0x1fe2: p1fe2SwimCalling(r); break;
        case 0x2068: p2068SwimWaving(r); break;
        case 0x20c1: p20c1SwimBoarding(r); break;
        default: {
            char buf[64];
            std::snprintf(buf, sizeof buf, "passenger state handler %04x not ported", addr);
            problems.push_back(buf);
        }
    }
}

// ---------------------------------------------------------------- helpers

/** Next entry of the frame table at descriptor+table, restarting at 0xffff. */
void Sim::nextFrame(Regs& r, int table) {
    while (true) {
        r.di = w16(u(r.si + table) + u(P_ANIM + r.bx));
        r.ax = u(r.di);
        if (r.ax != 0xffff) break;
        setD(P_ANIM + r.bx, 0);
    }
    setD(P_SPRITE + r.bx, r.ax);
}

/** DEC word [off]; true when it became zero. */
bool Sim::decZero(int off) {
    int v = w16(u(off) - 1);
    setD(off, v);
    return v == 0;
}

/** Animation delay; on expiry reloads it from descriptor+3e and steps the frame index. */
bool Sim::animTick(Regs& r) {
    if (!decZero(P_DELAY + r.bx)) return false;
    r.ax = u(r.si + 0x3e);
    setD(P_DELAY + r.bx, r.ax);
    addD(P_ANIM + r.bx, 2);
    return true;
}

void Sim::animReset(int bx) {
    setD(P_ANIM + bx, 0xfffe);
    setD(P_DELAY + bx, 1);
}

/** The passenger's feet reached the water: the descriptor's water set (+48). */
bool Sim::fellIntoWater(Regs& r) {
    r.ax = w16((s16(u(r.si + 2)) >> 1) + u(PPY + r.bx) - u(V_WATER_ROW));
    if (s16(r.ax) < 0) return false;
    r.ax = w16((u(V_WATER_ROW) + u(r.si + 2)) << 5);
    setD(PY + r.bx, r.ax);
    r.di = u(P_FROM2 + r.bx);
    r.ax = 0xffff;
    setD(PAD_WAITING + r.di, r.ax);
    setD(P_BUBBLE + r.bx, r.ax);
    switchToWaterSet(r);
    return true;
}

void Sim::switchToWaterSet(Regs& r) {
    r.si = u(r.si + 0x48);
    setD(P_DESC + r.bx, r.si);
    jumpVia(r, 0x08);
}

/** Knocked down by a copter flying through (2276 finds a player whose copter is not landed). */
bool Sim::hitByCopter(Regs& r) {
    if (!touchesPlayer(r)) return false;
    if (r.ax != 0xffff) return false;
    switchToWaterSet(r);
    return true;
}

/**
 * 113b:2276 - Passengers.kt touchesPlayer: is a copter touching the passenger (box from the descriptor:
 * +0 x offset, +2 y offset, +4 half width, +6 half height)? DI = player * 2, AX = its landed pad.
 */
bool Sim::touchesPlayer(Regs& r) {
    r.di = 0;
    while (true) {
        int si = r.si;
        bool ok = true;
        r.ax = w16((u(si + 2) << 5) + u(PY + r.bx));
        if (s16(r.ax) < d(P_YF + r.di)) ok = false;
        if (ok) {
            r.ax = w16((((u(si + 2) - u(si + 6) - u(si + 6)) << 5) + u(PY + r.bx)) - 0x280);
            if (s16(r.ax) > d(P_YF + r.di)) ok = false;
        }
        if (ok) {
            r.ax = w16((((u(si) + u(si + 4)) << 5) + u(PX + r.bx)) - 0xa0);
            if (s16(r.ax) < d(P_XF + r.di)) ok = false;
        }
        if (ok) {
            r.ax = w16((((u(si) - u(si + 4)) << 5) + u(PX + r.bx)) - 0x340);
            if (s16(r.ax) <= d(P_XF + r.di)) {
                r.ax = u(P_LANDED + r.di);
                return true;
            }
        }
        r.di = w16(r.di + 2);
        if (r.di != u(PLAYERS)) break;
    }
    r.di = 0xffff;
    return false;
}

/** Is a player landed on pad [pad]? Leaves DI at the player; -1 if none. */
int Sim::playerOnPad(Regs& r, int pad) {
    r.di = 0;
    while (true) {
        if (pad == u(P_LANDED + r.di)) return r.di;
        r.di = w16(r.di + 2);
        if (r.di == u(PLAYERS2)) return -1;
    }
}

// ---------------------------------------------------------------- the walking passenger

/** 113b:149c - takes the next stop of the route, or leaves the level when the route is finished. */
void Sim::p149cNextStop(Regs& r) {
    r.di = u(P_ROUTE + r.bx);
    r.ax = u(r.di + 4);
    if (r.ax == 0xffff) {
        int left = b8(d8(0x28f1) - 1);
        if ((left & 0x80) == 0) {
            setD8(0x28f1, left);
            if (left == 0) {  // last passenger done: end of level
                r.ax = 0xfffe;
                setD(V_FADE_STEP, r.ax);
                addD(V_FADE, r.ax);
                setD8(0x27cf, r.ax & 0xff);
            }
        }
        setD(P_SPRITE + r.bx, 0xffff);
        r.si = u(P_DESC + r.bx);
        r.ax = u(r.si + 0x2c);
        setD(P_STATE + r.bx, r.ax);
        passengerState(r.ax, r);
        return;
    }
    setD(P_TO + r.bx, r.ax);
    setD(P_TO2 + r.bx, r.ax << 1);
    r.ax = u(r.di);
    setD(P_FROM + r.bx, r.ax);
    setD(P_FROM2 + r.bx, r.ax << 1);
    r.ax = times3half(u(r.di + 2));
    setD(P_TIMER + r.bx, r.ax);
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x0a);
    setD(P_STATE + r.bx, r.ax);
}

/** 113b:1509 - hidden, waits for the timer and a free pickup pad. */
void Sim::p1509Arriving(Regs& r) {
    setD(P_SPRITE + r.bx, 0xffff);
    r.ax = u(P_TIMER + r.bx);
    if (r.ax != 0) {
        r.ax = w16(r.ax - 1);
        setD(P_TIMER + r.bx, r.ax);
        if (r.ax != 0) return;
    }
    r.di = u(P_FROM2 + r.bx);
    if (u(PAD_WAITING + r.di) != 0xffff) return;
    r.si = u(P_DESC + r.bx);
    jumpVia(r, 0x0c);
}

/** 113b:153b - appears at the door of the pickup pad. */
void Sim::p153bAppear(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x0e);
    setD(P_STATE + r.bx, r.ax);
    r.di = u(P_FROM2 + r.bx);
    setD(PAD_WAITING + r.di, r.bx);
    r.ax = w16(-u(r.si + 2) + u(PAD_Y + r.di));
    setD(PPY + r.bx, r.ax);
    r.ax = w16(r.ax << 5);
    setD(PY + r.bx, r.ax);
    r.ax = w16(-u(r.si) + u(PAD_W3 + r.di));
    setD(PPX + r.bx, r.ax);
    r.ax = w16(r.ax << 5);
    setD(PX + r.bx, r.ax);
    animReset(r.bx);
}

/** 113b:1582 - door animation (descriptor+36), then +10. */
void Sim::p1582Appearing(Regs& r) {
    if (!decZero(P_DELAY + r.bx)) return;
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x3e);
    setD(P_DELAY + r.bx, r.ax);
    addD(P_ANIM + r.bx, 2);
    r.di = w16(u(r.si + 0x36) + u(P_ANIM + r.bx));
    r.ax = u(r.di);
    if (r.ax == 0xffff) { jumpVia(r, 0x10); return; }
    setD(P_SPRITE + r.bx, r.ax);
}

/** 113b:15b4 */
void Sim::p15b4StartWaiting(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x12);
    setD(P_STATE + r.bx, r.ax);
    animReset(r.bx);
    r.ax = 0xffff;
    setD(P_COUNT + r.bx, r.ax);
    setD(P_BUBBLE + r.bx, r.ax);
}

/** 113b:15d7 - walks to the waiting spot of the pad (pad w4) and waits for a copter. */
void Sim::p15d7Waiting(Regs& r) {
    r.di = u(P_FROM2 + r.bx);
    r.si = u(P_DESC + r.bx);
    if (fellIntoWater(r)) return;
    if (decZero(P_DELAY + r.bx)) {
        r.ax = u(r.si + 0x3e);
        setD(P_DELAY + r.bx, r.ax);
        addD(P_ANIM + r.bx, 2);
        r.ax = w16(u(PPX + r.bx) + u(r.si));
        int target = d(PAD_W4 + r.di);
        if (r.ax == u(PAD_W4 + r.di)) {
            if (u(P_COUNT + r.bx) != 1) setD(P_ANIM + r.bx, 0);
            while (true) {
                r.di = w16(u(r.si + 0x2e) + u(P_ANIM + r.bx));
                r.ax = u(r.di);
                if (r.ax != 0xffff) break;
                setD(P_ANIM + r.bx, 0);
            }
            setD(P_SPRITE + r.bx, r.ax);
            setD(P_COUNT + r.bx, 1);
        } else {
            setD(P_COUNT + r.bx, 0);
            if (s16(r.ax) < target) { nextFrame(r, 0x34); addD(PX + r.bx, 0x20); }
            else { nextFrame(r, 0x32); addD(PX + r.bx, -0x20); }
        }
    }
    if (hitByCopter(r)) return;
    r.ax = u(P_FROM + r.bx);
    r.di = 0;
    while (true) {
        if (r.ax == u(P_LANDED + r.di) && u(PL_CARRY + r.di) == 0) { jumpVia(r, 0x14); return; }
        r.di = w16(r.di + 2);
        if (r.di == u(PLAYERS2)) return;
    }
}

/** 113b:16f6 - shows the destination bubble. */
void Sim::p16f6StartCalling(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x16);
    setD(P_STATE + r.bx, r.ax);
    animReset(r.bx);
    r.ax = w16(u(P_TO + r.bx) + 0x10c);
    if (r.ax >= 0x112) r.ax = 0x111;
    setD(P_BUBBLE + r.bx, r.ax);
    setD(P_COUNT + r.bx, 0x8c);
}

/** 113b:172a - a copter is on the pad: calls it for a while. */
void Sim::p172aCalling(Regs& r) {
    r.si = u(P_DESC + r.bx);
    if (fellIntoWater(r)) return;
    if (hitByCopter(r)) return;
    r.ax = u(P_FROM + r.bx);
    int p = playerOnPad(r, r.ax);
    if (p < 0 || u(PL_CARRY + r.di) != 0) { jumpVia(r, 0x18); return; }
    if (animTick(r)) nextFrame(r, 0x30);
    if (decZero(P_COUNT + r.bx)) jumpVia(r, 0x1c);
}

/** 113b:17e6 */
void Sim::p17e6StartImpatient(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x1a);
    setD(P_STATE + r.bx, r.ax);
    animReset(r.bx);
    setD(P_BUBBLE + r.bx, 0x112);
    setD(P_COUNT + r.bx, 0x8c);
}

/** 113b:180a - the copter left without it: waves impatiently, then waits again or calls the next copter. */
void Sim::p180aImpatient(Regs& r) {
    r.si = u(P_DESC + r.bx);
    if (fellIntoWater(r)) return;
    if (hitByCopter(r)) return;
    if (animTick(r)) nextFrame(r, 0x30);
    if (!decZero(P_COUNT + r.bx)) return;
    r.di = u(P_FROM2 + r.bx);
    r.ax = u(P_FROM + r.bx);
    r.di = 0;
    while (true) {
        if (r.ax == u(P_LANDED + r.di) && u(PL_CARRY + r.di) == 0) { jumpVia(r, 0x14); return; }
        r.di = w16(r.di + 2);
        if (r.di == u(PLAYERS2)) break;
    }
    jumpVia(r, 0x10);
}

/** 113b:18c8 */
void Sim::p18c8StartBoarding(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x1e);
    setD(P_STATE + r.bx, r.ax);
    animReset(r.bx);
    setD(P_BUBBLE + r.bx, 0xffff);
}

/** Walks towards the copter of player DI; boards (descriptor+20) when level with it. */
void Sim::walkToCopter(Regs& r) {
    r.cx = 0;
    if (decZero(P_DELAY + r.bx)) {
        r.ax = u(r.si + 0x3e);
        setD(P_DELAY + r.bx, r.ax);
        addD(P_ANIM + r.bx, 2);
        r.cx = 1;
    }
    r.ax = w16(u(PPX + r.bx) + u(r.si) - 0x10);
    int copterX = d(P_X + r.di);
    if (s16(r.ax) > copterX) { nextFrame(r, 0x32); if (r.cx != 0) addD(PX + r.bx, -0x20); }
    else if (s16(r.ax) < copterX) { nextFrame(r, 0x34); if (r.cx != 0) addD(PX + r.bx, 0x20); }
    else jumpVia(r, 0x20);
}

/** 113b:18e6 - walks to the landed copter. */
void Sim::p18e6Boarding(Regs& r) {
    r.si = u(P_DESC + r.bx);
    if (fellIntoWater(r)) return;
    if (hitByCopter(r)) return;
    r.ax = u(P_FROM + r.bx);
    if (playerOnPad(r, r.ax) < 0) { jumpVia(r, 0x18); return; }
    walkToCopter(r);
}

/** 113b:19e0 / 19fb - boards the copter of player DI: fare from the descriptor, target pad shown. */
void Sim::p19e0Board(Regs& r, bool switchDescriptor) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x40);
    setD(PL_FARE + r.di, r.ax);
    r.ax = u(r.si + 0x42);
    setD(PL_FARE_MIN + r.di, r.ax);
    if (switchDescriptor) {
        r.si = u(r.si + 0x48);
        setD(P_DESC + r.bx, r.si);
    }
    // 113b:1a0d
    r.ax = u(r.si + 0x22);
    setD(P_STATE + r.bx, r.ax);
    r.ax = u(r.si + 0x46);
    setD(PL_CARRY + r.di, r.ax);
    r.si = u(P_TO2 + r.bx);
    r.ax = u(PAD_NUMBER + r.si);
    setD(PL_TARGET + r.di, r.ax);
    setD(P_COUNT + r.bx, r.di);
    setD(P_SPRITE + r.bx, 0xffff);
    setD(P_BONUS + r.bx, 0xc8);
    r.di = u(P_FROM2 + r.bx);
    setD(PAD_WAITING + r.di, 0xffff);
}

/** 113b:1a42 - riding: the fare drops to its minimum, the bonus timer runs, until landing on the target pad. */
void Sim::p1a42Riding(Regs& r) {
    r.di = u(P_COUNT + r.bx);
    r.ax = u(PL_FARE + r.di);
    if (r.ax > u(PL_FARE_MIN + r.di)) { r.ax = w16(r.ax - 1); setD(PL_FARE + r.di, r.ax); }
    r.ax = u(P_BONUS + r.bx);
    if (s16(r.ax) > 0) { r.ax = w16(r.ax - 1); setD(P_BONUS + r.bx, r.ax); }
    r.ax = u(P_TO + r.bx);
    if (r.ax == u(P_LANDED + r.di)) {
        r.si = u(P_DESC + r.bx);
        jumpVia(r, 0x24);
    }
}

/** 113b:1a7e - delivered: pays fare * multiplier; a quick delivery drops a bonus item. */
void Sim::p1a7ePaid(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x26);
    setD(P_STATE + r.bx, r.ax);
    r.ax = 0;
    setD(PL_CARRY + r.di, 0);
    setD(PL_TARGET + r.di, 0);
    r.ax = w16((0x10 - u(r.si)) << 5);
    r.di = u(P_COUNT + r.bx);
    r.ax = w16(r.ax + u(P_XF + r.di));
    setD(PX + r.bx, r.ax);
    int64_t product = static_cast<int64_t>(u(PL_FARE + r.di)) * u(MULTIPLIER);
    r.ax = static_cast<int>(product & 0xffff);
    r.dx = static_cast<int>((product >> 16) & 0xffff);
    int lo = u(SCORE_LO) + r.ax;
    setD(SCORE_LO, lo);
    setD(SCORE_HI, u(SCORE_HI) + r.dx + (lo >> 16));
    if (u(P_BONUS + r.bx) != 0) {
        int savedSi = r.si;
        r.ax = u(MULTIPLIER);
        r.si = u(V_DIFFICULTY) << 1;
        if (r.ax < u(MULTIPLIER_LIMITS + r.si)) {
            r.ax = w16(u(P_XF + r.di) + 0x200);
            r.bp = w16(u(P_YF + r.di) + 0x140);
            r.cx = 0;
            r.dx = 0;
            r.si = BONUS_DESCRIPTOR;
            bonusSpawn(r);
        }
        r.si = savedSi;
    }
    r.di = u(P_TO2 + r.bx);
    r.ax = w16((-u(r.si + 2) + u(PAD_Y + r.di)) << 5);
    setD(PY + r.bx, r.ax);
    setD(P_ANIM + r.bx, 0xfffe);
    r.ax = 1;
    setD(P_DELAY + r.bx, 1);
    setD8(0x27cd, 1);
}

/** 113b:1b29 - walks from the copter to the house of the target pad (pad w3). */
void Sim::p1b29WalkingAway(Regs& r) {
    r.di = u(P_TO2 + r.bx);
    r.si = u(P_DESC + r.bx);
    if (!decZero(P_DELAY + r.bx)) return;
    r.ax = u(r.si + 0x3e);
    setD(P_DELAY + r.bx, r.ax);
    addD(P_ANIM + r.bx, 2);
    r.ax = w16(u(PPX + r.bx) + u(r.si));
    int door = d(PAD_W3 + r.di);
    if (r.ax == u(PAD_W3 + r.di)) { jumpVia(r, 0x28); return; }
    if (s16(r.ax) < door) { nextFrame(r, 0x34); addD(PX + r.bx, 0x20); }
    else { nextFrame(r, 0x32); addD(PX + r.bx, -0x20); }
    r.di = u(P_TO2 + r.bx);
    r.cx = 0;
    r.ax = w16((s16(u(PX + r.bx)) >> 5) + u(r.si));
    if (r.ax == u(PAD_W3 + r.di)) jumpVia(r, 0x28);
}

/** 113b:1bbe */
void Sim::p1bbeStartEntering(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x2a);
    setD(P_STATE + r.bx, r.ax);
    animReset(r.bx);
}

/** 113b:1bd6 - door animation (descriptor+38); then the next stop of the route. */
void Sim::p1bd6Entering(Regs& r) {
    if (!decZero(P_DELAY + r.bx)) return;
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x3e);
    setD(P_DELAY + r.bx, r.ax);
    addD(P_ANIM + r.bx, 2);
    r.di = w16(u(r.si + 0x38) + u(P_ANIM + r.bx));
    r.ax = u(r.di);
    setD(P_SPRITE + r.bx, r.ax);
    if (u(r.di + 2) != 0xffff) return;
    addD(P_ROUTE + r.bx, 4);
    jumpVia(r, 0x08);
}

// ---------------------------------------------------------------- the standing passenger (type 78dc)

/** 113b:1c0f */
void Sim::p1c0fStartStanding(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x12);
    setD(P_STATE + r.bx, r.ax);
    animReset(r.bx);
}

/** 113b:1c27 - waits to be grabbed by a copter that carries nobody. */
void Sim::p1c27Standing(Regs& r) {
    r.si = u(P_DESC + r.bx);
    if (touchesPlayer(r) && u(PL_CARRY + r.di) == 0) { jumpVia(r, 0x20); return; }
    setD(P_SPRITE + r.bx, 0x220);
}

/** 113b:1c48 */
void Sim::p1c48Grabbed(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x22);
    setD(P_STATE + r.bx, r.ax);
    r.ax = u(r.si + 0x46);
    setD(PL_CARRY + r.di, r.ax);
    setD(PL_TARGET + r.di, 7);
    setD(P_COUNT + r.bx, r.di);
    setD(P_SPRITE + r.bx, 0xffff);
}

/** 113b:1c6b - hangs on the copter until its pilot presses fire. */
void Sim::p1c6bHanging(Regs& r) {
    r.di = u(P_COUNT + r.bx);
    if ((d8(0x279c + r.di) & 0x80) == 0) return;
    r.si = u(P_DESC + r.bx);
    jumpVia(r, 0x24);
}

/** 113b:1c81 - released: falls with the copter's momentum. */
void Sim::p1c81Dropped(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = 0;
    setD(PL_CARRY + r.di, 0);
    setD(PL_TARGET + r.di, 0);
    r.ax = w16(s16(u(P_VX + r.di)) >> 5);
    setD(P_TIMER + r.bx, r.ax);
    r.ax = w16(s16(u(P_VY + r.di)) >> 5);
    setD(PVY + r.bx, r.ax);
    r.ax = w16(u(P_XF + r.di) + 0x200);
    r.bp = w16(u(P_YF + r.di) + 0x140);
    r.di = w16(u(r.si) << 5);
    r.ax = w16(r.ax - r.di);
    setD(PX + r.bx, r.ax);
    r.di = w16(u(r.si + 2) << 4);
    r.bp = w16(r.bp - r.di);
    setD(PY + r.bx, r.bp);
    setD(P_SPRITE + r.bx, 0x221);
    r.ax = u(r.si + 0x26);
    setD(P_STATE + r.bx, r.ax);
}

/** 113b:1cee - falling; lands on a pad (+10) or disappears off screen (+2c). */
void Sim::p1ceeFalling(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = w16(u(P_TIMER + r.bx) + u(PX + r.bx));
    if (s16(r.ax) <= -0x200 || s16(r.ax) >= 0x2800) { gone(r); return; }
    setD(PX + r.bx, r.ax);
    r.ax = w16(u(PVY + r.bx) + 2);
    setD(PVY + r.bx, r.ax);
    if (s16(r.ax) < 0) { addD(PY + r.bx, r.ax); return; }
    r.cx = u(PY + r.bx);
    r.ax = w16(r.ax + r.cx);
    if (s16(r.ax) >= 0x1800) { gone(r); return; }
    setD(PY + r.bx, r.ax);
    r.ax = w16((s16(r.ax) >> 5) + u(r.si + 2));
    r.cx = w16((s16(r.cx) >> 5) + u(r.si + 2));
    r.di = 0;
    while (d(PAD_LEFT + r.di) >= 0) {
        int padY = d(PAD_Y + r.di);
        if (s16(r.cx) <= padY && s16(r.ax) >= padY) {
            r.dx = w16((s16(u(PX + r.bx)) >> 5) + u(r.si));
            if (s16(r.dx) >= d(PAD_LEFT + r.di) && s16(r.dx) <= d(PAD_RIGHT + r.di)) {
                r.ax = w16((u(PAD_Y + r.di) - u(r.si + 2)) << 5);
                setD(PY + r.bx, r.ax);
                jumpVia(r, 0x10);
                return;
            }
        }
        r.di = w16(r.di + 2);
    }
}

void Sim::gone(Regs& r) {
    setD(P_SPRITE + r.bx, 0xffff);
    r.ax = u(r.si + 0x2c);
    setD(P_STATE + r.bx, r.ax);
}

// ---------------------------------------------------------------- the water set (descriptors at +48)

/** 113b:1da8 */
void Sim::p1da8StartSplash(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x0a);
    setD(P_STATE + r.bx, r.ax);
    r.di = u(P_FROM2 + r.bx);
    r.ax = 0xffff;
    setD(PAD_WAITING + r.di, r.ax);
    setD(P_BUBBLE + r.bx, r.ax);
    animReset(r.bx);
    setD(PVY + r.bx, 0);
}

/** 113b:1dd5 (1de2 = entry with the delay already expired) - splashes into the water and floats up. */
void Sim::p1dd5Splash(Regs& r, bool advance) {
    bool step = advance;
    if (!advance) {
        r.si = u(P_DESC + r.bx);
        step = decZero(P_DELAY + r.bx);
    }
    if (step) {
        r.ax = u(r.si + 0x3e);
        setD(P_DELAY + r.bx, r.ax);
        addD(P_ANIM + r.bx, 2);
    }
    nextFrame(r, 0x2e);
    r.cx = w16(u(PPY + r.bx) - u(r.si + 2) - u(V_WATER_ROW));
    int below = s16(r.cx);
    int ch;
    if (below == 0) ch = 0;
    else if (below < 0) ch = static_cast<int8_t>(below >> 8);
    else { r.cx = (r.cx & 0xff) | 0x100; ch = 1; }
    r.ax = u(PVY + r.bx);
    if (ch < 0) r.ax = w16(r.ax + 0x27);
    else if (s16(r.ax) > 0) r.ax = w16(r.ax - 0x175);
    else r.ax = w16(r.ax - 0x5c);
    r.ax = w16(std::clamp(s16(r.ax), -0x1800, 0x1800));
    setD(PVY + r.bx, r.ax);
    r.ax = w16((s16(r.ax) >> 6) + u(PY + r.bx));
    setD(PY + r.bx, r.ax);
    if (d(PVY + r.bx) >= 0) return;
    r.ax = w16((s16(r.ax) >> 5) - u(r.si + 2));
    if (s16(r.ax) > d(V_WATER_ROW)) return;
    r.ax = w16((u(V_WATER_ROW) - u(r.si + 2)) << 5);
    setD(PY + r.bx, r.ax);
    jumpVia(r, 0x10);
}

/** 113b:1e9c */
void Sim::p1e9cStartSinking(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x0e);
    setD(P_STATE + r.bx, r.ax);
    setD(P_BUBBLE + r.bx, 0xffff);
    animReset(r.bx);
    setD(PVY + r.bx, 0);
}

/** 113b:1ec0 - drowns: sinks until off the bottom (+2c). */
void Sim::p1ec0Sinking(Regs& r) {
    r.si = u(P_DESC + r.bx);
    animTick(r);
    nextFrame(r, 0x2e);
    r.ax = w16(u(PVY + r.bx) + 0x27);
    setD(PVY + r.bx, r.ax);
    r.ax = w16((s16(r.ax) >> 6) + u(PY + r.bx));
    if (r.ax < 0x1800) { setD(PY + r.bx, r.ax); return; }
    r.ax = u(r.si + 0x2c);
    setD(P_STATE + r.bx, r.ax);
    setD(P_SPRITE + r.bx, 0xffff);
}

/** 113b:1f24 */
void Sim::p1f24StartSwimming(Regs& r) {
    r.si = u(P_DESC + r.bx);
    r.ax = u(r.si + 0x12);
    setD(P_STATE + r.bx, r.ax);
    animReset(r.bx);
    r.ax = u(r.si + 0x44);
    setD(P_TIMER + r.bx, r.ax);
}

/** Keeps a swimmer on the water surface. */
void Sim::floatOnSurface(Regs& r) {
    r.ax = u(V_WATER_ROW);
    if (r.ax != u(PPY + r.bx)) {
        r.ax = w16((r.ax - u(r.si + 2)) << 5);
        setD(PY + r.bx, r.ax);
    }
}

/** 113b:1f43 - swims; a copter floating nearby (+14) rescues it, otherwise it drowns when the timer ends. */
void Sim::p1f43Swimming(Regs& r) {
    r.si = u(P_DESC + r.bx);
    animTick(r);
    nextFrame(r, 0x30);
    floatOnSurface(r);
    if (r.si != 0x77fe) {
        r.di = 0;
        while (true) {
            if (u(P_VY + r.di) == 0 && u(PL_CARRY + r.di) == 0) {
                r.cx = w16(u(P_Y + r.di) - u(V_WATER_ROW) + 0x12);
                if (r.cx == 0) { jumpVia(r, 0x14); return; }
            }
            r.di = w16(r.di + 2);
            if (r.di >= 4) break;
            if (u(PLAYERS) != 2) break;
        }
    }
    if (decZero(P_TIMER + r.bx)) jumpVia(r, 0x0c);
}

/** 113b:1fe2 - a copter is on the water: waves (+1c when done), or back to swimming (+18). */
void Sim::p1fe2SwimCalling(Regs& r) {
    r.si = u(P_DESC + r.bx);
    floatOnSurface(r);
    r.cx = w16(u(0x27dc) - u(V_WATER_ROW) + 0x12);
    if (r.cx != 0) {
        bool other = false;
        if (u(PLAYERS) == 2) {
            r.cx = w16(u(0x27de) - u(V_WATER_ROW) + 0x12);
            other = r.cx == 0;
        }
        if (!other) { jumpVia(r, 0x18); return; }
    }
    if (animTick(r)) nextFrame(r, 0x30);
    if (decZero(P_COUNT + r.bx)) jumpVia(r, 0x1c);
}

/** 113b:2068 - waves for a while (+0c when done). */
void Sim::p2068SwimWaving(Regs& r) {
    r.si = u(P_DESC + r.bx);
    floatOnSurface(r);
    if (animTick(r)) nextFrame(r, 0x30);
    if (decZero(P_COUNT + r.bx)) jumpVia(r, 0x0c);
}

/** 113b:20c1 - swims to the copter on the water. */
void Sim::p20c1SwimBoarding(Regs& r) {
    r.si = u(P_DESC + r.bx);
    floatOnSurface(r);
    r.di = 0;
    while (true) {
        if (u(PL_CARRY + r.di) == 0) {
            r.cx = w16(u(P_Y + r.di) - u(V_WATER_ROW) + 0x12);
            if (r.cx == 0) { walkToCopter(r); return; }
        }
        r.di = w16(r.di + 2);
        if (r.di >= 4) break;
        if (u(PLAYERS) != 2) break;
    }
    jumpVia(r, 0x18);
}

}  // namespace ugh
