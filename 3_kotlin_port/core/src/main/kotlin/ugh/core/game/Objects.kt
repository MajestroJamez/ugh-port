package ugh.core.game

import ugh.core.game.Game.Companion.s16

/*
 * Active objects (level list C, max 5) - port of 113b:2363 .. 113b:2b7e, 2196 and 22f1.
 * Arrays (index = object * 2, 10 bytes per array):
 *   2cad descriptor  2cb7 pad  2cc1 x  2ccb y (1/32 px)  2cd5 animation table / bonus list  2cdf start delay
 *   2ce9 x speed  2cf3 state handler  2cfd timer  2d07/2d11 x px (now/previous)  2d1b/2d25 y px
 *   2d2f animation index  2d39 animation delay  2d43 sprite  2d4d previous sprite
 *   2d57 facing (0 = left, 2 = right) / target player  2d61 handle of the looping flap sound
 *
 * Types: 0x7630 flying enemy (enters from a screen edge at the height of a player, deadly to its copter),
 * 0x766c walker on a pad (turns to a landed copter, charges and throws it), 0x76a8 blower (pushes copters
 * sideways), 0x76e4 tree (catches a falling passenger and drops a bonus item from its list).
 * A dropped passenger falling on an enemy stuns it and scores descriptor+3a.
 */

private const val O_DESC = 0x2cad
private const val O_PAD = 0x2cb7
private const val O_X = 0x2cc1
private const val O_Y = 0x2ccb
private const val O_TABLE = 0x2cd5
private const val O_DELAY0 = 0x2cdf
private const val O_VX = 0x2ce9
private const val O_STATE = 0x2cf3
private const val O_TIMER = 0x2cfd
private const val O_XPX = 0x2d07
private const val O_YPX = 0x2d1b
private const val O_ANIM = 0x2d2f
private const val O_ADELAY = 0x2d39
private const val O_SPRITE = 0x2d43
private const val O_FACING = 0x2d57
private const val O_SOUND = 0x2d61

private const val SFX_SCREECH = 0x425b
private const val SFX_FLAP = 0x4260
private const val SFX_BLOW = 0x4265
private const val SFX_BONUS = 0x4274

/** 113b:2363 - runs the state handler of every object. */
fun Game.objectsUpdate(r: Regs = Regs()) {
    r.bx = 0
    while (u(O_DESC + r.bx) != 0xffff) {
        objectState(u(O_STATE + r.bx), r)
        r.bx = (r.bx + 2) and 0xffff
    }
}

fun Game.objectState(addr: Int, r: Regs) {
    when (addr) {
        0x2379 -> o2379FlyerInit(r)
        0x239f -> o239fFlyerWait(r)
        0x23b0 -> o23b0FlyerScreech(r)
        0x23d9 -> o23d9FlyerWait2(r)
        0x23ea -> o23eaFlyerStart(r)
        0x2493 -> o2493Flying(r)
        0x252b -> o252bFlyerHit(r)
        0x255e -> o255eFlyerFalling(r)
        0x25b1 -> o25b1WalkerInit(r)
        0x25c9 -> o25c9Walking(r)
        0x2667 -> o2667StartWatching(r)
        0x2681 -> o2681Watching(r)
        0x272e -> o272eStartCharging(r)
        0x2748 -> o2748Charging(r)
        0x2830, 0x288f -> o2830StartRecovering(r)
        0x2844 -> o2844Recovering(r, 0x2a)
        0x28a3 -> o2844Recovering(r, 0x32)
        0x28ee -> o28eeWalkerStunned(r)
        0x2914 -> o2914Stunned(r)
        0x295b -> o295bBlowerInit(r)
        0x2973 -> o2973Blowing(r)
        0x2a53 -> o2a53BlowerStunned(r)
        0x2a76 -> o2a76BlowerWait(r)
        0x2a87 -> o2a87TreeInit(r)
        0x2ab5 -> o2ab5Tree(r)
        0x2b0c -> o2b0cTreeCatch(r)
        0x2b58 -> o2b58TreeWait(r)
        0x2b7e, 0x1c0e -> {}
        else -> error("object state handler %04x not ported (%s)".format(addr, r))
    }
}

private fun Game.jumpObj(r: Regs, off: Int) = objectState(u(r.si + off), r)

private fun Game.decZeroO(off: Int): Boolean {
    val v = (u(off) - 1) and 0xffff
    setD(off, v)
    return v == 0
}

private fun Game.addScore(v: Int) {
    val lo = u(0x261e) + v
    setD(0x261e, lo)
    setD(0x2620, u(0x2620) + (lo shr 16))
}

/** Animation step through the table at DGROUP:[table] (+ index), restarting at 0xffff. */
private fun Game.objFrame(r: Regs, table: Int) {
    while (true) {
        r.di = (table + u(O_ANIM + r.bx)) and 0xffff
        r.ax = u(r.di)
        if (r.ax != 0xffff) break
        setD(O_ANIM + r.bx, 0)
    }
    setD(O_SPRITE + r.bx, r.ax)
}

/** Table of the current facing: word at descriptor + 2d57 + [off] (MOV DI,DS:[BP+SI+off]). */
private fun Game.facingTable(r: Regs, off: Int): Int {
    r.bp = u(O_FACING + r.bx)
    return u(r.bp + r.si + off)
}

/** A falling passenger hits the object: it bounces up (NEG vy) and gets the "hit" sprite. */
private fun Game.passengerHitsObject(r: Regs, setSprite: Boolean = true): Boolean {
    if (!fallingPassengerNear(r)) return false
    setD(0x2c6d + r.di, -u(0x2c6d + r.di))
    if (setSprite) setD(0x2bed + r.di, 0x222)
    return true
}

/** Is a player landed on the object's pad? Leaves DI at the player (or at DGROUP:2636). */
private fun Game.playerOnObjectPad(r: Regs): Boolean {
    r.ax = u(O_PAD + r.bx)
    r.di = 0
    while (true) {
        if (r.ax == u(Game.P_LANDED + r.di)) return true
        r.di = (r.di + 2) and 0xffff
        if (r.di == u(0x2636)) return false
    }
}

/** Faces the copter of player DI (26f7 / 27b2): facing 2 and positive speed when the copter is to the right. */
private fun Game.faceCopter(r: Regs) {
    r.cx = u(O_VX + r.bx)
    r.ax = u(O_X + r.bx)
    if (s16(r.ax) < d(Game.P_XF + r.di)) {
        r.ax = 2
        if (s16(r.cx) < 0) r.cx = (-r.cx) and 0xffff
    } else {
        r.ax = 0
        if (s16(r.cx) >= 0) r.cx = (-r.cx) and 0xffff
    }
    setD(O_FACING + r.bx, r.ax)
    setD(O_VX + r.bx, r.cx)
}

// ---------------------------------------------------------------- flying enemy (7630)

private fun Game.o2379FlyerInit(r: Regs) {
    r.si = u(O_DESC + r.bx)
    r.ax = u(r.si + 0x0a)
    setD(O_STATE + r.bx, r.ax)
    setD(O_ANIM + r.bx, 0xfffe)
    setD(O_ADELAY + r.bx, 1)
    setD(O_SPRITE + r.bx, 0xffff)
    r.ax = u(O_DELAY0 + r.bx)
    setD(O_TIMER + r.bx, r.ax)
}

private fun Game.o239fFlyerWait(r: Regs) {
    if (!decZeroO(O_TIMER + r.bx)) return
    r.si = u(O_DESC + r.bx)
    jumpObj(r, 0x0c)
}

private fun Game.o23b0FlyerScreech(r: Regs) {
    r.si = u(O_DESC + r.bx)
    r.ax = u(r.si + 0x0e)
    setD(O_STATE + r.bx, r.ax)
    setD(O_TIMER + r.bx, 0x46)
    playEffect(r, SFX_SCREECH)
}

private fun Game.o23d9FlyerWait2(r: Regs) {
    if (!decZeroO(O_TIMER + r.bx)) return
    r.si = u(O_DESC + r.bx)
    jumpObj(r, 0x10)
}

/** 113b:23ea - picks the next player as target and enters from the far side at its height. */
private fun Game.o23eaFlyerStart(r: Regs) {
    r.si = u(O_DESC + r.bx)
    r.di = u(O_FACING + r.bx) xor 1
    if (s16(r.di) >= d(0x2634)) r.di = 0
    setD(O_FACING + r.bx, r.di)
    r.di = (r.di shl 1) and 0xffff
    if (d(Game.P_XF + r.di) < 0x1400) {
        r.ax = u(O_VX + r.bx)
        if (s16(r.ax) >= 0) { r.ax = (-r.ax) and 0xffff; setD(O_VX + r.bx, r.ax) }
        r.ax = 0x27e0
        r.cx = u(r.si + 0x22)
    } else {
        r.ax = u(O_VX + r.bx)
        if (s16(r.ax) < 0) { r.ax = (-r.ax) and 0xffff; setD(O_VX + r.bx, r.ax) }
        r.ax = 0xfc20
        r.cx = u(r.si + 0x24)
    }
    setD(O_X + r.bx, r.ax)
    setD(O_TABLE + r.bx, r.cx)
    r.ax = (u(Game.P_YF + r.di) + 0x340) and 0xffff
    if (s16(r.ax) > d(0x28fe)) r.ax = u(0x28fe)
    r.ax = (r.ax - 0x340) and 0xffff
    if (s16(r.ax) < -0x80) r.ax = 0xff80
    setD(O_Y + r.bx, r.ax)
    r.ax = u(r.si + 0x12)
    setD(O_STATE + r.bx, r.ax)
    playEffect(r, SFX_FLAP, flags = 0)
    setD(O_SOUND + r.bx, r.ax)
}

private fun Game.stopFlap(r: Regs) = soundStop(u(O_SOUND + r.bx))

/** 113b:2493 - flies across the screen; touching its target's copter ends the life. */
private fun Game.o2493Flying(r: Regs) {
    r.si = u(O_DESC + r.bx)
    r.ax = (u(O_VX + r.bx) + u(O_X + r.bx)) and 0xffff
    if (s16(r.ax) <= -0x400 || s16(r.ax) >= 0x2800) { stopFlap(r); jumpObj(r, 0x08); return }
    setD(O_X + r.bx, r.ax)
    if (!decZeroO(O_ADELAY + r.bx)) return
    setD(O_ADELAY + r.bx, 4)
    addD(O_ANIM + r.bx, 2)
    objFrame(r, u(O_TABLE + r.bx))
    if (fallingPassengerNear(r)) {
        stopFlap(r)
        setD(0x2c6d + r.di, -u(0x2c6d + r.di))
        setD(0x2bed + r.di, 0x222)
        jumpObj(r, 0x1c)
        return
    }
    if (!copterTouchesObject(r)) return
    r.di = r.di shr 1
    if (r.di != u(O_FACING + r.bx)) return
    if (d(Game.V_FADE_STEP) < 0) return
    r.ax = 0xfffe
    setD(Game.V_FADE_STEP, r.ax)
    addD(Game.V_FADE, r.ax)
}

private fun Game.o252bFlyerHit(r: Regs) {
    r.ax = u(r.si + 0x1e)
    setD(O_STATE + r.bx, r.ax)
    r.ax = u(r.si + 0x3a)
    addScore(r.ax)
    setD(O_TIMER + r.bx, 0)
    r.si = if (s16(u(O_VX + r.bx)) < 0) u(r.si + 0x36) else u(r.si + 0x38)
    r.ax = u(r.si)
    setD(O_SPRITE + r.bx, r.ax)
}

private fun Game.o255eFlyerFalling(r: Regs) {
    r.ax = (u(O_VX + r.bx) + u(O_X + r.bx)) and 0xffff
    if (s16(r.ax) <= -0x400 || s16(r.ax) >= 0x2800) { r.si = u(O_DESC + r.bx); jumpObj(r, 0x08); return }
    setD(O_X + r.bx, r.ax)
    r.ax = u(O_TIMER + r.bx)
    if (s16(r.ax) < 0x28) { r.ax = (r.ax + 1) and 0xffff; setD(O_TIMER + r.bx, r.ax) }
    r.ax = (r.ax + u(O_Y + r.bx)) and 0xffff
    if (s16(r.ax) >= 0x1800) { r.si = u(O_DESC + r.bx); jumpObj(r, 0x08); return }
    setD(O_Y + r.bx, r.ax)
}

// ---------------------------------------------------------------- walker on a pad (766c)

private fun Game.o25b1WalkerInit(r: Regs) {
    r.si = u(O_DESC + r.bx)
    r.ax = u(r.si + 0x0a)
    setD(O_STATE + r.bx, r.ax)
    setD(O_ANIM + r.bx, 0xfffe)
    setD(O_ADELAY + r.bx, 1)
}

/** 113b:25c9 - walks along its pad, turning at the ends; watches for landing copters. */
private fun Game.o25c9Walking(r: Regs) {
    r.si = u(O_DESC + r.bx)
    if (decZeroO(O_ADELAY + r.bx)) {
        r.di = (u(O_PAD + r.bx) shl 1) and 0xffff
        r.ax = (u(O_VX + r.bx) + u(O_X + r.bx)) and 0xffff
        setD(O_X + r.bx, r.ax)
        r.ax = (s16(r.ax) shr 5) and 0xffff
        var turn = s16(r.ax) < d(Game.PAD_LEFT + r.di)
        if (!turn) {
            r.ax = (r.ax + 0x20) and 0xffff
            turn = s16(r.ax) >= d(Game.PAD_RIGHT + r.di)
        }
        if (turn) {
            setD(O_VX + r.bx, -u(O_VX + r.bx))
            setD(O_FACING + r.bx, u(O_FACING + r.bx) xor 2)
        }
        setD(O_ADELAY + r.bx, 4)
        addD(O_ANIM + r.bx, 2)
        while (true) {
            r.bp = u(O_FACING + r.bx)
            r.di = (u(r.bp + r.si + 0x22) + u(O_ANIM + r.bx)) and 0xffff
            r.ax = u(r.di)
            if (r.ax != 0xffff) break
            setD(O_ANIM + r.bx, 0)
        }
        setD(O_SPRITE + r.bx, r.ax)
    }
    if (playerOnObjectPad(r)) { jumpObj(r, 0x0c); return }
    if (passengerHitsObject(r)) jumpObj(r, 0x1c)
}

private fun Game.o2667StartWatching(r: Regs) {
    r.ax = u(r.si + 0x0e)
    setD(O_STATE + r.bx, r.ax)
    setD(O_ANIM + r.bx, 0xfffe)
    setD(O_ADELAY + r.bx, 1)
    setD(O_TIMER + r.bx, 0x8c)
}

/** 113b:2681 - a copter landed on its pad: turns to it, then charges (+10). */
private fun Game.o2681Watching(r: Regs) {
    r.si = u(O_DESC + r.bx)
    if (decZeroO(O_TIMER + r.bx)) { jumpObj(r, 0x10); return }
    if (decZeroO(O_ADELAY + r.bx)) {
        setD(O_ADELAY + r.bx, 5)
        addD(O_ANIM + r.bx, 2)
        while (true) {
            r.di = (facingTable(r, 0x2e) + u(O_ANIM + r.bx)) and 0xffff
            r.ax = u(r.di)
            if (r.ax != 0xffff) break
            setD(O_ANIM + r.bx, 0)
        }
        setD(O_SPRITE + r.bx, r.ax)
    }
    if (passengerHitsObject(r)) { jumpObj(r, 0x1c); return }
    if (!playerOnObjectPad(r)) { jumpObj(r, 0x08); return }
    faceCopter(r)
}

private fun Game.o272eStartCharging(r: Regs) {
    r.ax = u(r.si + 0x12)
    setD(O_STATE + r.bx, r.ax)
    setD(O_ANIM + r.bx, 0xfffe)
    setD(O_ADELAY + r.bx, 1)
    setD(O_TIMER + r.bx, 0)
}

/** 113b:2748 - charges at the copter, speeding up; hitting it throws the copter into the air (+18). */
private fun Game.o2748Charging(r: Regs) {
    r.si = u(O_DESC + r.bx)
    if (decZeroO(O_ADELAY + r.bx)) {
        setD(O_ADELAY + r.bx, 4)
        addD(O_ANIM + r.bx, 2)
        while (true) {
            r.di = (facingTable(r, 0x26) + u(O_ANIM + r.bx)) and 0xffff
            r.ax = u(r.di)
            if (r.ax != 0xffff) break
            setD(O_ANIM + r.bx, 0)
        }
        setD(O_SPRITE + r.bx, r.ax)
    }
    if (passengerHitsObject(r)) { jumpObj(r, 0x1c); return }
    if (!playerOnObjectPad(r)) { jumpObj(r, 0x14); return }
    faceCopter(r)
    r.ax = (u(O_FACING + r.bx) - 1 + u(O_TIMER + r.bx)) and 0xffff
    setD(O_TIMER + r.bx, r.ax)
    r.ax = (r.ax + u(O_VX + r.bx) + u(O_X + r.bx)) and 0xffff
    setD(O_X + r.bx, r.ax)
    if (!copterTouchesObject(r)) return
    addD(Game.P_YF + r.di, -0x20)
    r.ax = (u(O_VX + r.bx) + u(O_TIMER + r.bx)) and 0xffff
    setD(Game.P_VX + r.di, r.ax shl 5)
    r.ax = (r.ax shl 4) and 0xffff
    setD(Game.P_VY + r.di, r.ax)
    setD(Game.P_LANDED + r.di, 0xffff)
    jumpObj(r, 0x18)
}

private fun Game.o2830StartRecovering(r: Regs) {
    r.ax = u(r.si + 0x16)
    setD(O_STATE + r.bx, r.ax)
    setD(O_ANIM + r.bx, 0xfffe)
    setD(O_ADELAY + r.bx, 1)
}

/** 113b:2844 / 28a3 - one animation run (table +2a / +32), then walking again (+8). */
private fun Game.o2844Recovering(r: Regs, table: Int) {
    r.si = u(O_DESC + r.bx)
    if (decZeroO(O_ADELAY + r.bx)) {
        setD(O_ADELAY + r.bx, 4)
        addD(O_ANIM + r.bx, 2)
        r.di = (facingTable(r, table) + u(O_ANIM + r.bx)) and 0xffff
        r.ax = u(r.di)
        if (r.ax == 0xffff) { jumpObj(r, 0x08); return }
        setD(O_SPRITE + r.bx, r.ax)
    }
    if (passengerHitsObject(r)) jumpObj(r, 0x1c)
}

private fun Game.o28eeWalkerStunned(r: Regs) {
    r.ax = u(r.si + 0x1e)
    setD(O_STATE + r.bx, r.ax)
    r.ax = u(r.si + 0x3a)
    addScore(r.ax)
    setD(O_ANIM + r.bx, 0xfffe)
    setD(O_ADELAY + r.bx, 1)
    setD(O_TIMER + r.bx, 0x15e)
}

private fun Game.o2914Stunned(r: Regs) {
    r.si = u(O_DESC + r.bx)
    if (decZeroO(O_TIMER + r.bx)) { jumpObj(r, 0x08); return }
    if (!decZeroO(O_ADELAY + r.bx)) return
    setD(O_ADELAY + r.bx, 4)
    addD(O_ANIM + r.bx, 2)
    while (true) {
        r.di = (facingTable(r, 0x36) + u(O_ANIM + r.bx)) and 0xffff
        r.ax = u(r.di)
        if (r.ax != 0xffff) break
        setD(O_ANIM + r.bx, 0)
    }
    setD(O_SPRITE + r.bx, r.ax)
}

// ---------------------------------------------------------------- blower (76a8)

private fun Game.o295bBlowerInit(r: Regs) {
    r.si = u(O_DESC + r.bx)
    r.ax = u(r.si + 0x12)
    setD(O_STATE + r.bx, r.ax)
    setD(O_ANIM + r.bx, 0xfffe)
    setD(O_ADELAY + r.bx, 1)
}

/** 113b:2973 - blows: copters in front of it are pushed sideways, direction depends on the animation phase. */
private fun Game.o2973Blowing(r: Regs) {
    r.si = u(O_DESC + r.bx)
    if (decZeroO(O_ADELAY + r.bx)) {
        setD(O_ADELAY + r.bx, 0x0f)
        addD(O_ANIM + r.bx, 2)
        if (u(O_ANIM + r.bx) == 6) playEffect(r, SFX_BLOW)
        objFrame(r, u(r.si + 0x26))
    }
    r.di = 0
    while (true) {
        var inside = true
        r.ax = ((((u(r.si + 2) - 9) shl 5)) + u(O_Y + r.bx)) and 0xffff
        if (s16(r.ax) < d(Game.P_YF + r.di)) inside = false
        if (inside) {
            r.ax = (((u(r.si + 2) - 0x1e) shl 5) + u(O_Y + r.bx)) and 0xffff
            if (s16(r.ax) > d(Game.P_YF + r.di)) inside = false
        }
        if (inside) {
            r.ax = (((u(r.si) - 0x10) shl 5) + u(O_X + r.bx)) and 0xffff
            if (s16(r.ax) < d(Game.P_XF + r.di)) inside = false
        }
        if (inside) {
            r.ax = ((((u(r.si) - 0x68) shl 5) + u(O_X + r.bx)) - 0x340) and 0xffff
            if (s16(r.ax) > d(Game.P_XF + r.di)) inside = false
        }
        if (inside) {
            r.ax = if (s16(u(O_ANIM + r.bx)) < 5) 0x29 else 0xffd7
            addD(Game.P_VX + r.di, r.ax)
        }
        r.di = (r.di + 2) and 0xffff
        if (r.di != u(0x2634)) break
    }
    if (passengerHitsObject(r, setSprite = false)) jumpObj(r, 0x1c)
}

private fun Game.o2a53BlowerStunned(r: Regs) {
    r.ax = u(r.si + 0x1e)
    setD(O_STATE + r.bx, r.ax)
    r.ax = u(r.si + 0x3a)
    addScore(r.ax)
    setD(O_TIMER + r.bx, 0x15e)
    r.si = u(r.si + 0x36)
    r.ax = u(r.si)
    setD(O_SPRITE + r.bx, r.ax)
}

private fun Game.o2a76BlowerWait(r: Regs) {
    if (!decZeroO(O_TIMER + r.bx)) return
    r.si = u(O_DESC + r.bx)
    jumpObj(r, 0x10)
}

// ---------------------------------------------------------------- tree (76e4)

private fun Game.o2a87TreeInit(r: Regs) {
    r.si = u(O_DESC + r.bx)
    r.ax = u(r.si + 0x0a)
    setD(O_STATE + r.bx, r.ax)
    setD(O_ANIM + r.bx, 0xfffe)
    setD(O_ADELAY + r.bx, 1)
    r.ax = (u(O_X + r.bx) shl 5) and 0xffff
    setD(O_XPX + r.bx, r.ax)
    r.ax = (u(O_Y + r.bx) shl 5) and 0xffff
    setD(O_YPX + r.bx, r.ax)
}

/** 113b:2ab5 - sways; a falling passenger bounces off it (+1c: drops a bonus). */
private fun Game.o2ab5Tree(r: Regs) {
    if (!decZeroO(O_ADELAY + r.bx)) return
    r.si = u(O_DESC + r.bx)
    setD(O_ADELAY + r.bx, 6)
    addD(O_ANIM + r.bx, 2)
    objFrame(r, u(r.si + 0x22))
    if (!fallingPassengerNear(r)) return
    r.ax = s16(-u(0x2c6d + r.di)) shr 1 and 0xffff
    setD(0x2c6d + r.di, r.ax)
    setD(0x2bed + r.di, 0x222)
    r.si = u(O_DESC + r.bx)
    jumpObj(r, 0x1c)
}

/** 113b:2b0c - drops the next bonus item of its list (DGROUP:[2cd5]) where the passenger hit it. */
private fun Game.o2b0cTreeCatch(r: Regs) {
    r.ax = u(r.si + 0x1e)
    setD(O_STATE + r.bx, r.ax)
    setD(O_TIMER + r.bx, 0xd2)
    setD(O_SPRITE + r.bx, 0xe8)
    r.ax = u(0x2aed + r.di)
    r.bp = u(0x2b0d + r.di)
    r.cx = u(0x2acd + r.di)
    r.dx = (s16(-u(0x2c6d + r.di)) shr 2) and 0xffff
    r.si = u(O_TABLE + r.bx)
    r.si = u(r.si)
    bonusState(u(r.si + 4), r)
    addD(O_TABLE + r.bx, 2)
    playEffect(r, SFX_BONUS)
}

private fun Game.o2b58TreeWait(r: Regs) {
    r.si = u(O_DESC + r.bx)
    if (!decZeroO(O_TIMER + r.bx)) return
    r.di = u(O_TABLE + r.bx)
    r.ax = if (u(r.di) == 0xffff) u(r.si + 0x20) else u(r.si + 0x0a)
    setD(O_STATE + r.bx, r.ax)
}

// ---------------------------------------------------------------- proximity tests

/**
 * 113b:2196 - a dropped standing passenger (type 78dc, state 1cee, falling down) close to object BX.
 * DI = passenger*2 or 0xffff.
 */
fun Game.fallingPassengerNear(r: Regs): Boolean {
    r.di = 0
    while (true) {
        r.ax = u(0x29ad + r.di)
        if (r.ax == 0xffff) break
        if (r.ax == 0x78dc && u(0x2a0d + r.di) == 0x1cee && d(0x2c6d + r.di) >= 0) {
            r.ax = (u(0x2b0d + r.di) + 0x100) and 0xffff
            if (s16(r.ax) >= d(O_Y + r.bx)) {
                r.ax = (r.ax - 0x380) and 0xffff
                if (s16(r.ax) <= d(O_Y + r.bx)) {
                    r.ax = (u(0x2aed + r.di) + 0x180) and 0xffff
                    if (s16(r.ax) >= d(O_X + r.bx)) {
                        r.ax = (r.ax - 0x4c0) and 0xffff
                        if (s16(r.ax) <= d(O_X + r.bx)) return true
                    }
                }
            }
        }
        r.di = (r.di + 2) and 0xffff
    }
    r.di = 0xffff
    return false
}

/** 113b:22f1 - a copter touching object BX (box from descriptor SI). DI = player*2 or 0xffff. */
fun Game.copterTouchesObject(r: Regs): Boolean {
    r.di = 0
    while (true) {
        val si = r.si
        var ok = true
        r.ax = ((u(si + 2) shl 5) + u(O_Y + r.bx)) and 0xffff
        if (s16(r.ax) < d(Game.P_YF + r.di)) ok = false
        if (ok) {
            r.ax = ((((u(si + 2) - u(si + 6) - u(si + 6)) shl 5) + u(O_Y + r.bx)) - 0x280) and 0xffff
            if (s16(r.ax) > d(Game.P_YF + r.di)) ok = false
        }
        if (ok) {
            r.ax = ((((u(si) + u(si + 4)) shl 5) + u(O_X + r.bx)) - 0xa0) and 0xffff
            if (s16(r.ax) < d(Game.P_XF + r.di)) ok = false
        }
        if (ok) {
            r.ax = ((((u(si) - u(si + 4)) shl 5) + u(O_X + r.bx)) - 0x340) and 0xffff
            if (s16(r.ax) <= d(Game.P_XF + r.di)) return true
        }
        r.di = (r.di + 2) and 0xffff
        if (r.di != u(0x2634)) break
    }
    r.di = 0xffff
    return false
}
