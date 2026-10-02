// Active objects (level list C, max 5) - 113b:2363 .. 113b:2b7e, 2196 and 22f1, ported from Objects.kt.
//
// Arrays (index = object * 2, 10 bytes per array): 2cad descriptor, 2cb7 pad, 2cc1 / 2ccb x / y (1/32 px),
// 2cd5 animation table / bonus list, 2cdf start delay, 2ce9 x speed, 2cf3 state handler, 2cfd timer,
// 2d2f animation index, 2d39 animation delay, 2d43 sprite, 2d57 facing (0 = left, 2 = right) / target player,
// 2d61 handle of the looping flap sound. Types: 0x7630 flying enemy, 0x766c walker on a pad, 0x76a8 blower,
// 0x76e4 tree. The sound effects of the original (ADLX blocks 425b, 4260, 4265, 4274) are left out.
#include "sim.hpp"

#include <cstdio>

namespace ugh {

namespace {

constexpr int O_DESC = 0x2cad, O_PAD = 0x2cb7, O_X = 0x2cc1, O_Y = 0x2ccb, O_TABLE = 0x2cd5, O_DELAY0 = 0x2cdf;
constexpr int O_VX = 0x2ce9, O_STATE = 0x2cf3, O_TIMER = 0x2cfd, O_XPX = 0x2d07, O_YPX = 0x2d1b;
constexpr int O_ANIM = 0x2d2f, O_ADELAY = 0x2d39, O_SPRITE = 0x2d43, O_FACING = 0x2d57, O_SOUND = 0x2d61;

}  // namespace

/** 113b:2363 - Objects.kt objectsUpdate: runs the state handler of every object. */
void Sim::objectsUpdate(Regs& r) {
    r.bx = 0;
    while (u(O_DESC + r.bx) != 0xffff) {
        objectState(u(O_STATE + r.bx), r);
        r.bx = w16(r.bx + 2);
    }
}

void Sim::objectState(int addr, Regs& r) {
    switch (addr) {
        case 0x2379: o2379FlyerInit(r); break;
        case 0x239f: o239fFlyerWait(r); break;
        case 0x23b0: o23b0FlyerScreech(r); break;
        case 0x23d9: o23d9FlyerWait2(r); break;
        case 0x23ea: o23eaFlyerStart(r); break;
        case 0x2493: o2493Flying(r); break;
        case 0x252b: o252bFlyerHit(r); break;
        case 0x255e: o255eFlyerFalling(r); break;
        case 0x25b1: o25b1WalkerInit(r); break;
        case 0x25c9: o25c9Walking(r); break;
        case 0x2667: o2667StartWatching(r); break;
        case 0x2681: o2681Watching(r); break;
        case 0x272e: o272eStartCharging(r); break;
        case 0x2748: o2748Charging(r); break;
        case 0x2830: case 0x288f: o2830StartRecovering(r); break;
        case 0x2844: o2844Recovering(r, 0x2a); break;
        case 0x28a3: o2844Recovering(r, 0x32); break;
        case 0x28ee: o28eeWalkerStunned(r); break;
        case 0x2914: o2914Stunned(r); break;
        case 0x295b: o295bBlowerInit(r); break;
        case 0x2973: o2973Blowing(r); break;
        case 0x2a53: o2a53BlowerStunned(r); break;
        case 0x2a76: o2a76BlowerWait(r); break;
        case 0x2a87: o2a87TreeInit(r); break;
        case 0x2ab5: o2ab5Tree(r); break;
        case 0x2b0c: o2b0cTreeCatch(r); break;
        case 0x2b58: o2b58TreeWait(r); break;
        case 0x2b7e: case 0x1c0e: break;
        default: {
            char buf[64];
            std::snprintf(buf, sizeof buf, "object state handler %04x not ported", addr);
            problems.push_back(buf);
        }
    }
}

void Sim::addScore(int v) {
    int lo = u(0x261e) + v;
    setD(0x261e, lo);
    setD(0x2620, u(0x2620) + (lo >> 16));
}

/** Animation step through the table at DGROUP:[table] (+ index), restarting at 0xffff. */
void Sim::objFrame(Regs& r, int table) {
    while (true) {
        r.di = w16(table + u(O_ANIM + r.bx));
        r.ax = u(r.di);
        if (r.ax != 0xffff) break;
        setD(O_ANIM + r.bx, 0);
    }
    setD(O_SPRITE + r.bx, r.ax);
}

/** Table of the current facing: word at descriptor + facing + off (MOV DI,DS:[BP+SI+off]). */
int Sim::facingTable(Regs& r, int off) {
    r.bp = u(O_FACING + r.bx);
    return u(r.bp + r.si + off);
}

/** A falling passenger hits the object: it bounces up (NEG vy) and gets the "hit" sprite. */
bool Sim::passengerHitsObject(Regs& r, bool setSprite) {
    if (!fallingPassengerNear(r)) return false;
    setD(0x2c6d + r.di, -u(0x2c6d + r.di));
    if (setSprite) setD(0x2bed + r.di, 0x222);
    return true;
}

/** Is a player landed on the object's pad? Leaves DI at the player (or at DGROUP:2636). */
bool Sim::playerOnObjectPad(Regs& r) {
    r.ax = u(O_PAD + r.bx);
    r.di = 0;
    while (true) {
        if (r.ax == u(P_LANDED + r.di)) return true;
        r.di = w16(r.di + 2);
        if (r.di == u(PLAYERS2)) return false;
    }
}

/** Faces the copter of player DI: facing 2 and positive speed when the copter is to the right. */
void Sim::faceCopter(Regs& r) {
    r.cx = u(O_VX + r.bx);
    r.ax = u(O_X + r.bx);
    if (s16(r.ax) < d(P_XF + r.di)) {
        r.ax = 2;
        if (s16(r.cx) < 0) r.cx = w16(-r.cx);
    } else {
        r.ax = 0;
        if (s16(r.cx) >= 0) r.cx = w16(-r.cx);
    }
    setD(O_FACING + r.bx, r.ax);
    setD(O_VX + r.bx, r.cx);
}

// ---------------------------------------------------------------- flying enemy (7630)

void Sim::o2379FlyerInit(Regs& r) {
    r.si = u(O_DESC + r.bx);
    r.ax = u(r.si + 0x0a);
    setD(O_STATE + r.bx, r.ax);
    setD(O_ANIM + r.bx, 0xfffe);
    setD(O_ADELAY + r.bx, 1);
    setD(O_SPRITE + r.bx, 0xffff);
    r.ax = u(O_DELAY0 + r.bx);
    setD(O_TIMER + r.bx, r.ax);
}

void Sim::o239fFlyerWait(Regs& r) {
    if (!decZero(O_TIMER + r.bx)) return;
    r.si = u(O_DESC + r.bx);
    jumpObj(r, 0x0c);
}

void Sim::o23b0FlyerScreech(Regs& r) {
    r.si = u(O_DESC + r.bx);
    r.ax = u(r.si + 0x0e);
    setD(O_STATE + r.bx, r.ax);
    setD(O_TIMER + r.bx, 0x46);
}

void Sim::o23d9FlyerWait2(Regs& r) {
    if (!decZero(O_TIMER + r.bx)) return;
    r.si = u(O_DESC + r.bx);
    jumpObj(r, 0x10);
}

/** 113b:23ea - picks the next player as target and enters from the far side at its height. */
void Sim::o23eaFlyerStart(Regs& r) {
    r.si = u(O_DESC + r.bx);
    r.di = u(O_FACING + r.bx) ^ 1;
    if (s16(r.di) >= d(PLAYERS)) r.di = 0;
    setD(O_FACING + r.bx, r.di);
    r.di = w16(r.di << 1);
    if (d(P_XF + r.di) < 0x1400) {
        r.ax = u(O_VX + r.bx);
        if (s16(r.ax) >= 0) { r.ax = w16(-r.ax); setD(O_VX + r.bx, r.ax); }
        r.ax = 0x27e0;
        r.cx = u(r.si + 0x22);
    } else {
        r.ax = u(O_VX + r.bx);
        if (s16(r.ax) < 0) { r.ax = w16(-r.ax); setD(O_VX + r.bx, r.ax); }
        r.ax = 0xfc20;
        r.cx = u(r.si + 0x24);
    }
    setD(O_X + r.bx, r.ax);
    setD(O_TABLE + r.bx, r.cx);
    r.ax = w16(u(P_YF + r.di) + 0x340);
    if (s16(r.ax) > d(0x28fe)) r.ax = u(0x28fe);
    r.ax = w16(r.ax - 0x340);
    if (s16(r.ax) < -0x80) r.ax = 0xff80;
    setD(O_Y + r.bx, r.ax);
    r.ax = u(r.si + 0x12);
    setD(O_STATE + r.bx, r.ax);
    setD(O_SOUND + r.bx, 0);  // the handle of the looping flap sound
}

/** 113b:2493 - flies across the screen; touching its target's copter ends the life. */
void Sim::o2493Flying(Regs& r) {
    r.si = u(O_DESC + r.bx);
    r.ax = w16(u(O_VX + r.bx) + u(O_X + r.bx));
    if (s16(r.ax) <= -0x400 || s16(r.ax) >= 0x2800) { jumpObj(r, 0x08); return; }
    setD(O_X + r.bx, r.ax);
    if (!decZero(O_ADELAY + r.bx)) return;
    setD(O_ADELAY + r.bx, 4);
    addD(O_ANIM + r.bx, 2);
    objFrame(r, u(O_TABLE + r.bx));
    if (fallingPassengerNear(r)) {
        setD(0x2c6d + r.di, -u(0x2c6d + r.di));
        setD(0x2bed + r.di, 0x222);
        jumpObj(r, 0x1c);
        return;
    }
    if (!copterTouchesObject(r)) return;
    r.di = r.di >> 1;
    if (r.di != u(O_FACING + r.bx)) return;
    if (d(V_FADE_STEP) < 0) return;
    r.ax = 0xfffe;
    setD(V_FADE_STEP, r.ax);
    addD(V_FADE, r.ax);
}

void Sim::o252bFlyerHit(Regs& r) {
    r.ax = u(r.si + 0x1e);
    setD(O_STATE + r.bx, r.ax);
    r.ax = u(r.si + 0x3a);
    addScore(r.ax);
    setD(O_TIMER + r.bx, 0);
    r.si = s16(u(O_VX + r.bx)) < 0 ? u(r.si + 0x36) : u(r.si + 0x38);
    r.ax = u(r.si);
    setD(O_SPRITE + r.bx, r.ax);
}

void Sim::o255eFlyerFalling(Regs& r) {
    r.ax = w16(u(O_VX + r.bx) + u(O_X + r.bx));
    if (s16(r.ax) <= -0x400 || s16(r.ax) >= 0x2800) { r.si = u(O_DESC + r.bx); jumpObj(r, 0x08); return; }
    setD(O_X + r.bx, r.ax);
    r.ax = u(O_TIMER + r.bx);
    if (s16(r.ax) < 0x28) { r.ax = w16(r.ax + 1); setD(O_TIMER + r.bx, r.ax); }
    r.ax = w16(r.ax + u(O_Y + r.bx));
    if (s16(r.ax) >= 0x1800) { r.si = u(O_DESC + r.bx); jumpObj(r, 0x08); return; }
    setD(O_Y + r.bx, r.ax);
}

// ---------------------------------------------------------------- walker on a pad (766c)

void Sim::o25b1WalkerInit(Regs& r) {
    r.si = u(O_DESC + r.bx);
    r.ax = u(r.si + 0x0a);
    setD(O_STATE + r.bx, r.ax);
    setD(O_ANIM + r.bx, 0xfffe);
    setD(O_ADELAY + r.bx, 1);
}

/** 113b:25c9 - walks along its pad, turning at the ends; watches for landing copters. */
void Sim::o25c9Walking(Regs& r) {
    r.si = u(O_DESC + r.bx);
    if (decZero(O_ADELAY + r.bx)) {
        r.di = w16(u(O_PAD + r.bx) << 1);
        r.ax = w16(u(O_VX + r.bx) + u(O_X + r.bx));
        setD(O_X + r.bx, r.ax);
        r.ax = w16(s16(r.ax) >> 5);
        bool turn = s16(r.ax) < d(PAD_LEFT + r.di);
        if (!turn) {
            r.ax = w16(r.ax + 0x20);
            turn = s16(r.ax) >= d(PAD_RIGHT + r.di);
        }
        if (turn) {
            setD(O_VX + r.bx, -u(O_VX + r.bx));
            setD(O_FACING + r.bx, u(O_FACING + r.bx) ^ 2);
        }
        setD(O_ADELAY + r.bx, 4);
        addD(O_ANIM + r.bx, 2);
        while (true) {
            r.bp = u(O_FACING + r.bx);
            r.di = w16(u(r.bp + r.si + 0x22) + u(O_ANIM + r.bx));
            r.ax = u(r.di);
            if (r.ax != 0xffff) break;
            setD(O_ANIM + r.bx, 0);
        }
        setD(O_SPRITE + r.bx, r.ax);
    }
    if (playerOnObjectPad(r)) { jumpObj(r, 0x0c); return; }
    if (passengerHitsObject(r)) jumpObj(r, 0x1c);
}

void Sim::o2667StartWatching(Regs& r) {
    r.ax = u(r.si + 0x0e);
    setD(O_STATE + r.bx, r.ax);
    setD(O_ANIM + r.bx, 0xfffe);
    setD(O_ADELAY + r.bx, 1);
    setD(O_TIMER + r.bx, 0x8c);
}

/** 113b:2681 - a copter landed on its pad: turns to it, then charges (+10). */
void Sim::o2681Watching(Regs& r) {
    r.si = u(O_DESC + r.bx);
    if (decZero(O_TIMER + r.bx)) { jumpObj(r, 0x10); return; }
    if (decZero(O_ADELAY + r.bx)) {
        setD(O_ADELAY + r.bx, 5);
        addD(O_ANIM + r.bx, 2);
        while (true) {
            r.di = w16(facingTable(r, 0x2e) + u(O_ANIM + r.bx));
            r.ax = u(r.di);
            if (r.ax != 0xffff) break;
            setD(O_ANIM + r.bx, 0);
        }
        setD(O_SPRITE + r.bx, r.ax);
    }
    if (passengerHitsObject(r)) { jumpObj(r, 0x1c); return; }
    if (!playerOnObjectPad(r)) { jumpObj(r, 0x08); return; }
    faceCopter(r);
}

void Sim::o272eStartCharging(Regs& r) {
    r.ax = u(r.si + 0x12);
    setD(O_STATE + r.bx, r.ax);
    setD(O_ANIM + r.bx, 0xfffe);
    setD(O_ADELAY + r.bx, 1);
    setD(O_TIMER + r.bx, 0);
}

/** 113b:2748 - charges at the copter, speeding up; hitting it throws the copter into the air (+18). */
void Sim::o2748Charging(Regs& r) {
    r.si = u(O_DESC + r.bx);
    if (decZero(O_ADELAY + r.bx)) {
        setD(O_ADELAY + r.bx, 4);
        addD(O_ANIM + r.bx, 2);
        while (true) {
            r.di = w16(facingTable(r, 0x26) + u(O_ANIM + r.bx));
            r.ax = u(r.di);
            if (r.ax != 0xffff) break;
            setD(O_ANIM + r.bx, 0);
        }
        setD(O_SPRITE + r.bx, r.ax);
    }
    if (passengerHitsObject(r)) { jumpObj(r, 0x1c); return; }
    if (!playerOnObjectPad(r)) { jumpObj(r, 0x14); return; }
    faceCopter(r);
    r.ax = w16(u(O_FACING + r.bx) - 1 + u(O_TIMER + r.bx));
    setD(O_TIMER + r.bx, r.ax);
    r.ax = w16(r.ax + u(O_VX + r.bx) + u(O_X + r.bx));
    setD(O_X + r.bx, r.ax);
    if (!copterTouchesObject(r)) return;
    addD(P_YF + r.di, -0x20);
    r.ax = w16(u(O_VX + r.bx) + u(O_TIMER + r.bx));
    setD(P_VX + r.di, r.ax << 5);
    r.ax = w16(r.ax << 4);
    setD(P_VY + r.di, r.ax);
    setD(P_LANDED + r.di, 0xffff);
    jumpObj(r, 0x18);
}

void Sim::o2830StartRecovering(Regs& r) {
    r.ax = u(r.si + 0x16);
    setD(O_STATE + r.bx, r.ax);
    setD(O_ANIM + r.bx, 0xfffe);
    setD(O_ADELAY + r.bx, 1);
}

/** 113b:2844 / 28a3 - one animation run (table +2a / +32), then walking again (+8). */
void Sim::o2844Recovering(Regs& r, int table) {
    r.si = u(O_DESC + r.bx);
    if (decZero(O_ADELAY + r.bx)) {
        setD(O_ADELAY + r.bx, 4);
        addD(O_ANIM + r.bx, 2);
        r.di = w16(facingTable(r, table) + u(O_ANIM + r.bx));
        r.ax = u(r.di);
        if (r.ax == 0xffff) { jumpObj(r, 0x08); return; }
        setD(O_SPRITE + r.bx, r.ax);
    }
    if (passengerHitsObject(r)) jumpObj(r, 0x1c);
}

void Sim::o28eeWalkerStunned(Regs& r) {
    r.ax = u(r.si + 0x1e);
    setD(O_STATE + r.bx, r.ax);
    r.ax = u(r.si + 0x3a);
    addScore(r.ax);
    setD(O_ANIM + r.bx, 0xfffe);
    setD(O_ADELAY + r.bx, 1);
    setD(O_TIMER + r.bx, 0x15e);
}

void Sim::o2914Stunned(Regs& r) {
    r.si = u(O_DESC + r.bx);
    if (decZero(O_TIMER + r.bx)) { jumpObj(r, 0x08); return; }
    if (!decZero(O_ADELAY + r.bx)) return;
    setD(O_ADELAY + r.bx, 4);
    addD(O_ANIM + r.bx, 2);
    while (true) {
        r.di = w16(facingTable(r, 0x36) + u(O_ANIM + r.bx));
        r.ax = u(r.di);
        if (r.ax != 0xffff) break;
        setD(O_ANIM + r.bx, 0);
    }
    setD(O_SPRITE + r.bx, r.ax);
}

// ---------------------------------------------------------------- blower (76a8)

void Sim::o295bBlowerInit(Regs& r) {
    r.si = u(O_DESC + r.bx);
    r.ax = u(r.si + 0x12);
    setD(O_STATE + r.bx, r.ax);
    setD(O_ANIM + r.bx, 0xfffe);
    setD(O_ADELAY + r.bx, 1);
}

/** 113b:2973 - blows: copters in front of it are pushed sideways, direction depends on the animation phase. */
void Sim::o2973Blowing(Regs& r) {
    r.si = u(O_DESC + r.bx);
    if (decZero(O_ADELAY + r.bx)) {
        setD(O_ADELAY + r.bx, 0x0f);
        addD(O_ANIM + r.bx, 2);
        objFrame(r, u(r.si + 0x26));
    }
    r.di = 0;
    while (true) {
        bool inside = true;
        r.ax = w16(((u(r.si + 2) - 9) << 5) + u(O_Y + r.bx));
        if (s16(r.ax) < d(P_YF + r.di)) inside = false;
        if (inside) {
            r.ax = w16(((u(r.si + 2) - 0x1e) << 5) + u(O_Y + r.bx));
            if (s16(r.ax) > d(P_YF + r.di)) inside = false;
        }
        if (inside) {
            r.ax = w16(((u(r.si) - 0x10) << 5) + u(O_X + r.bx));
            if (s16(r.ax) < d(P_XF + r.di)) inside = false;
        }
        if (inside) {
            r.ax = w16((((u(r.si) - 0x68) << 5) + u(O_X + r.bx)) - 0x340);
            if (s16(r.ax) > d(P_XF + r.di)) inside = false;
        }
        if (inside) {
            r.ax = s16(u(O_ANIM + r.bx)) < 5 ? 0x29 : 0xffd7;
            addD(P_VX + r.di, r.ax);
        }
        r.di = w16(r.di + 2);
        if (r.di != u(PLAYERS)) break;
    }
    if (passengerHitsObject(r, false)) jumpObj(r, 0x1c);
}

void Sim::o2a53BlowerStunned(Regs& r) {
    r.ax = u(r.si + 0x1e);
    setD(O_STATE + r.bx, r.ax);
    r.ax = u(r.si + 0x3a);
    addScore(r.ax);
    setD(O_TIMER + r.bx, 0x15e);
    r.si = u(r.si + 0x36);
    r.ax = u(r.si);
    setD(O_SPRITE + r.bx, r.ax);
}

void Sim::o2a76BlowerWait(Regs& r) {
    if (!decZero(O_TIMER + r.bx)) return;
    r.si = u(O_DESC + r.bx);
    jumpObj(r, 0x10);
}

// ---------------------------------------------------------------- tree (76e4)

void Sim::o2a87TreeInit(Regs& r) {
    r.si = u(O_DESC + r.bx);
    r.ax = u(r.si + 0x0a);
    setD(O_STATE + r.bx, r.ax);
    setD(O_ANIM + r.bx, 0xfffe);
    setD(O_ADELAY + r.bx, 1);
    r.ax = w16(u(O_X + r.bx) << 5);
    setD(O_XPX + r.bx, r.ax);
    r.ax = w16(u(O_Y + r.bx) << 5);
    setD(O_YPX + r.bx, r.ax);
}

/** 113b:2ab5 - sways; a falling passenger bounces off it (+1c: drops a bonus). */
void Sim::o2ab5Tree(Regs& r) {
    if (!decZero(O_ADELAY + r.bx)) return;
    r.si = u(O_DESC + r.bx);
    setD(O_ADELAY + r.bx, 6);
    addD(O_ANIM + r.bx, 2);
    objFrame(r, u(r.si + 0x22));
    if (!fallingPassengerNear(r)) return;
    r.ax = w16(s16(-u(0x2c6d + r.di)) >> 1);
    setD(0x2c6d + r.di, r.ax);
    setD(0x2bed + r.di, 0x222);
    r.si = u(O_DESC + r.bx);
    jumpObj(r, 0x1c);
}

/** 113b:2b0c - drops the next bonus item of its list (DGROUP:[2cd5]) where the passenger hit it. */
void Sim::o2b0cTreeCatch(Regs& r) {
    r.ax = u(r.si + 0x1e);
    setD(O_STATE + r.bx, r.ax);
    setD(O_TIMER + r.bx, 0xd2);
    setD(O_SPRITE + r.bx, 0xe8);
    r.ax = u(0x2aed + r.di);
    r.bp = u(0x2b0d + r.di);
    r.cx = u(0x2acd + r.di);
    r.dx = w16(s16(-u(0x2c6d + r.di)) >> 2);
    r.si = u(O_TABLE + r.bx);
    r.si = u(r.si);
    bonusState(u(r.si + 4), r);
    addD(O_TABLE + r.bx, 2);
}

void Sim::o2b58TreeWait(Regs& r) {
    r.si = u(O_DESC + r.bx);
    if (!decZero(O_TIMER + r.bx)) return;
    r.di = u(O_TABLE + r.bx);
    r.ax = u(r.di) == 0xffff ? u(r.si + 0x20) : u(r.si + 0x0a);
    setD(O_STATE + r.bx, r.ax);
}

// ---------------------------------------------------------------- proximity tests

/** 113b:2196 - a dropped standing passenger (type 78dc, falling down) close to object BX; DI = passenger * 2. */
bool Sim::fallingPassengerNear(Regs& r) {
    r.di = 0;
    while (true) {
        r.ax = u(0x29ad + r.di);
        if (r.ax == 0xffff) break;
        if (r.ax == 0x78dc && u(0x2a0d + r.di) == 0x1cee && d(0x2c6d + r.di) >= 0) {
            r.ax = w16(u(0x2b0d + r.di) + 0x100);
            if (s16(r.ax) >= d(O_Y + r.bx)) {
                r.ax = w16(r.ax - 0x380);
                if (s16(r.ax) <= d(O_Y + r.bx)) {
                    r.ax = w16(u(0x2aed + r.di) + 0x180);
                    if (s16(r.ax) >= d(O_X + r.bx)) {
                        r.ax = w16(r.ax - 0x4c0);
                        if (s16(r.ax) <= d(O_X + r.bx)) return true;
                    }
                }
            }
        }
        r.di = w16(r.di + 2);
    }
    r.di = 0xffff;
    return false;
}

/** 113b:22f1 - a copter touching object BX (box from descriptor SI); DI = player * 2 or 0xffff. */
bool Sim::copterTouchesObject(Regs& r) {
    r.di = 0;
    while (true) {
        int si = r.si;
        bool ok = true;
        r.ax = w16((u(si + 2) << 5) + u(O_Y + r.bx));
        if (s16(r.ax) < d(P_YF + r.di)) ok = false;
        if (ok) {
            r.ax = w16((((u(si + 2) - u(si + 6) - u(si + 6)) << 5) + u(O_Y + r.bx)) - 0x280);
            if (s16(r.ax) > d(P_YF + r.di)) ok = false;
        }
        if (ok) {
            r.ax = w16((((u(si) + u(si + 4)) << 5) + u(O_X + r.bx)) - 0xa0);
            if (s16(r.ax) < d(P_XF + r.di)) ok = false;
        }
        if (ok) {
            r.ax = w16((((u(si) - u(si + 4)) << 5) + u(O_X + r.bx)) - 0x340);
            if (s16(r.ax) <= d(P_XF + r.di)) return true;
        }
        r.di = w16(r.di + 2);
        if (r.di != u(PLAYERS)) break;
    }
    r.di = 0xffff;
    return false;
}

}  // namespace ugh
