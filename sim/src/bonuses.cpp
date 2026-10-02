// Bonus items (12 slots, index = slot * 2) - 113b:2b7f .. 113b:2d1b, ported from Bonuses.kt.
// Arrays: 2d6b descriptor, 2d83 state handler, 2d9b sprite (0xffff = free), 2dcb / 2de3 x / y (1/32 px),
// 2e5b x speed / lifetime, 2e73 y speed.
#include "sim.hpp"

#include <cstdio>

namespace ugh {

namespace {

constexpr int B_DESC = 0x2d6b, B_STATE = 0x2d83, B_SPRITE = 0x2d9b, B_X = 0x2dcb, B_Y = 0x2de3;
constexpr int B_VX = 0x2e5b, B_VY = 0x2e73;

}  // namespace

/**
 * 113b:2b96 - Bonuses.kt bonusSpawn: drops a bonus item, SI = descriptor, AX / BP = position, CX / DX = speed.
 * Original: with all 12 slots in use the routine returns without its POP BX and jumps to CS:BX.
 */
void Sim::bonusSpawn(Regs& r) {
    int savedBx = r.bx;
    r.bx = 0x16;
    while ((u(B_SPRITE + r.bx) & 0x8000) == 0) {
        r.bx = w16(r.bx - 2);
        if (s16(r.bx) < 0) {
            problems.push_back("all 12 bonus slots in use: the original would jump to CS:BX");
            r.bx = savedBx;
            return;
        }
    }
    setD(B_DESC + r.bx, r.si);
    r.di = w16(u(r.si) << 5);
    r.ax = w16(r.ax - r.di);
    setD(B_X + r.bx, r.ax);
    r.di = w16(u(r.si + 2) << 4);
    r.bp = w16(r.bp - r.di);
    setD(B_Y + r.bx, r.bp);
    setD(B_VX + r.bx, r.cx);
    r.dx = w16(-(r.dx + u(r.si + 0x0c)));
    setD(B_VY + r.bx, r.dx);
    r.ax = u(r.si + 0x10);
    setD(B_SPRITE + r.bx, r.ax);
    r.ax = u(r.si + 6);
    setD(B_STATE + r.bx, r.ax);
    r.bx = savedBx;
}

/** 113b:2b7f - Bonuses.kt bonusesUpdate: runs the state handler of every used bonus slot, last slot first. */
void Sim::bonusesUpdate(Regs& r) {
    r.bx = 0x16;
    while (true) {
        if (u(B_SPRITE + r.bx) != 0xffff) bonusState(u(B_STATE + r.bx), r);
        r.bx = w16(r.bx - 2);
        if (s16(r.bx) < 0) break;
    }
}

void Sim::bonusState(int addr, Regs& r) {
    switch (addr) {
        case 0x2b96: bonusSpawn(r); break;
        case 0x2be4: b2be4Falling(r); break;
        case 0x2c97: b2c97Landed(r); break;
        case 0x2ca9: b2ca9Lying(r); break;
        default: {
            char buf[64];
            std::snprintf(buf, sizeof buf, "bonus state handler %04x not ported", addr);
            problems.push_back(buf);
        }
    }
}

/** 113b:2be4 - flies and falls (gravity 3) until it lands on a pad (+8) or leaves the screen. */
void Sim::b2be4Falling(Regs& r) {
    r.si = u(B_DESC + r.bx);
    r.ax = w16(u(B_VX + r.bx) + u(B_X + r.bx));
    if (s16(r.ax) <= -0x200 || s16(r.ax) >= 0x2800) { setD(B_SPRITE + r.bx, 0xffff); return; }
    setD(B_X + r.bx, r.ax);
    r.ax = w16(u(B_VY + r.bx) + 3);
    setD(B_VY + r.bx, r.ax);
    if (s16(r.ax) < 0) { addD(B_Y + r.bx, r.ax); return; }
    r.cx = u(B_Y + r.bx);
    r.ax = w16(r.ax + r.cx);
    if (s16(r.ax) >= 0x1800) { setD(B_SPRITE + r.bx, 0xffff); return; }
    setD(B_Y + r.bx, r.ax);
    r.ax = w16((s16(r.ax) >> 5) + u(r.si + 2));
    r.cx = w16((s16(r.cx) >> 5) + u(r.si + 2));
    r.di = 0;
    while (d(PAD_LEFT + r.di) >= 0) {
        int padY = d(PAD_Y + r.di);
        if (s16(r.cx) < padY && s16(r.ax) >= padY) {
            r.dx = w16((s16(u(B_X + r.bx)) >> 5) + u(r.si));
            if (s16(r.dx) >= d(PAD_LEFT + r.di)) {
                r.dx = w16(r.dx - 1);
                if (s16(r.dx) <= d(PAD_RIGHT + r.di)) {
                    r.ax = w16((u(PAD_Y + r.di) - u(r.si + 2)) << 5);
                    setD(B_Y + r.bx, r.ax);
                    bonusState(u(r.si + 8), r);
                    return;
                }
            }
        }
        r.di = w16(r.di + 2);
    }
}

/** 113b:2c97 - landed: lies on the pad for 0x230 frames. */
void Sim::b2c97Landed(Regs& r) {
    r.si = u(B_DESC + r.bx);
    r.ax = u(r.si + 0x0a);
    setD(B_STATE + r.bx, r.ax);
    setD(B_VX + r.bx, 0x230);
}

/** 113b:2ca9 - lying; a copter touching it collects it (energy, a life, or the score multiplier). */
void Sim::b2ca9Lying(Regs& r) {
    int expired = w16(u(B_VX + r.bx) - 1);
    setD(B_VX + r.bx, expired);
    if (expired == 0) { setD(B_SPRITE + r.bx, 0xffff); return; }
    r.si = u(B_DESC + r.bx);
    if (!copterTouchesBonus(r)) return;
    r.ax = u(r.si + 0x12);
    if (r.ax == 0) {  // energy
        r.ax = w16(u(r.si + 0x0e) + u(V_ENERGY));
        if (r.ax > 0x5a3b) r.ax = 0x5a3b;
        setD(V_ENERGY, r.ax);
    } else if (r.ax == 1) {  // lives, at most 99
        r.ax = u(r.si + 0x0e);
        int al = b8((r.ax & 0xff) + d8(0x263c));
        if (al > 0x63) al = 0x63;
        r.ax = (r.ax & 0xff00) | al;
        setD8(0x263c, al);
    } else {  // score multiplier, limited by the difficulty
        r.ax = u(0x263d);
        r.di = u(V_DIFFICULTY) << 1;
        if (r.ax < u(0x2628 + r.di)) {
            int al = b8((r.ax & 0xff) + 1);
            r.ax = (r.ax & 0xff00) | al;
            setD8(0x263d, al);
        }
    }
    setD(B_SPRITE + r.bx, 0xffff);
}

/** 113b:2207 - is a copter touching bonus item BX (box from descriptor SI)? DI = player * 2 or 0xffff. */
bool Sim::copterTouchesBonus(Regs& r) {
    r.di = 0;
    while (true) {
        int si = r.si;
        bool ok = true;
        r.ax = w16((u(si + 2) << 5) + u(B_Y + r.bx));
        if (s16(r.ax) < d(P_YF + r.di)) ok = false;
        if (ok) {
            r.ax = w16((((u(si + 2) - 6) << 5) + u(B_Y + r.bx)) - 0x280);
            if (s16(r.ax) > d(P_YF + r.di)) ok = false;
        }
        if (ok) {
            r.ax = w16((((u(si) + 3) << 5) + u(B_X + r.bx)) - 0xa0);
            if (s16(r.ax) < d(P_XF + r.di)) ok = false;
        }
        if (ok) {
            r.ax = w16((((u(si) - 3) << 5) + u(B_X + r.bx)) - 0x340);
            if (s16(r.ax) <= d(P_XF + r.di)) return true;
        }
        r.di = w16(r.di + 2);
        if (r.di != u(PLAYERS)) break;
    }
    r.di = 0xffff;
    return false;
}

}  // namespace ugh
