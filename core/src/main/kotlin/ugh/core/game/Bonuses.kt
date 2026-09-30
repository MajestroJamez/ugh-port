package ugh.core.game

import ugh.core.game.Game.Companion.s16

/*
 * Bonus items (third object group) - port of 113b:2b7f .. 113b:2d1b and 113b:2207.
 * 12 slots, index = slot * 2, processed from 0x16 down to 0:
 *   2d6b descriptor  2d83 state handler  2d9b sprite (-1 = free)  2dcb x  2de3 y (1/32 px)
 *   2e5b x speed / lifetime  2e73 y speed
 * Descriptor (e.g. 7a38): +0 x offset, +2 y offset, +4 spawn 2b96, +6 falling 2be4, +8 landed 2c97,
 * +a lying 2ca9, +c y speed, +e amount, +10 sprite, +12 kind (0 energy, 1 lives, else multiplier).
 */

private const val B_DESC = 0x2d6b
private const val B_STATE = 0x2d83
private const val B_SPRITE = 0x2d9b
private const val B_X = 0x2dcb
private const val B_Y = 0x2de3
private const val B_VX = 0x2e5b
private const val B_VY = 0x2e73

/** 113b:2b7f - runs the state handler of every used bonus slot. */
fun Game.bonusesUpdate(r: Regs = Regs()) {
    r.bx = 0x16
    while (true) {
        if (u(B_SPRITE + r.bx) != 0xffff) bonusState(u(B_STATE + r.bx), r)
        r.bx = (r.bx - 2) and 0xffff
        if (s16(r.bx) < 0) break
    }
}

fun Game.bonusState(addr: Int, r: Regs) {
    when (addr) {
        0x2b96 -> bonusSpawn(r)
        0x2be4 -> b2be4Falling(r)
        0x2c97 -> b2c97Landed(r)
        0x2ca9 -> b2ca9Lying(r)
        else -> error("bonus state handler %04x not ported (%s)".format(addr, r))
    }
}

/**
 * 113b:2b96 - drops a bonus item: SI = descriptor, AX/BP = position, CX/DX = speed.
 * Original: with all 12 slots in use the routine returns without its POP BX and jumps to CS:BX.
 */
fun Game.bonusSpawn(r: Regs) {
    val savedBx = r.bx
    r.bx = 0x16
    while (u(B_SPRITE + r.bx) and 0x8000 == 0) {
        r.bx = (r.bx - 2) and 0xffff
        if (s16(r.bx) < 0) error("all 12 bonus slots in use: the original would jump to CS:%04x here".format(savedBx))
    }
    setD(B_DESC + r.bx, r.si)
    r.di = (u(r.si) shl 5) and 0xffff
    r.ax = (r.ax - r.di) and 0xffff
    setD(B_X + r.bx, r.ax)
    r.di = (u(r.si + 2) shl 4) and 0xffff
    r.bp = (r.bp - r.di) and 0xffff
    setD(B_Y + r.bx, r.bp)
    setD(B_VX + r.bx, r.cx)
    r.dx = (-(r.dx + u(r.si + 0x0c))) and 0xffff
    setD(B_VY + r.bx, r.dx)
    r.ax = u(r.si + 0x10)
    setD(B_SPRITE + r.bx, r.ax)
    r.ax = u(r.si + 6)
    setD(B_STATE + r.bx, r.ax)
    r.bx = savedBx
}

/** 113b:2be4 - flies and falls (gravity 3) until it lands on a pad (+8) or leaves the screen. */
private fun Game.b2be4Falling(r: Regs) {
    r.si = u(B_DESC + r.bx)
    r.ax = (u(B_VX + r.bx) + u(B_X + r.bx)) and 0xffff
    if (s16(r.ax) <= -0x200 || s16(r.ax) >= 0x2800) { setD(B_SPRITE + r.bx, 0xffff); return }
    setD(B_X + r.bx, r.ax)
    r.ax = (u(B_VY + r.bx) + 3) and 0xffff
    setD(B_VY + r.bx, r.ax)
    if (s16(r.ax) < 0) { addD(B_Y + r.bx, r.ax); return }
    r.cx = u(B_Y + r.bx)
    r.ax = (r.ax + r.cx) and 0xffff
    if (s16(r.ax) >= 0x1800) { setD(B_SPRITE + r.bx, 0xffff); return }
    setD(B_Y + r.bx, r.ax)
    r.ax = ((s16(r.ax) shr 5) + u(r.si + 2)) and 0xffff
    r.cx = ((s16(r.cx) shr 5) + u(r.si + 2)) and 0xffff
    r.di = 0
    while (d(Game.PAD_LEFT + r.di) >= 0) {
        val padY = d(Game.PAD_Y + r.di)
        if (s16(r.cx) < padY && s16(r.ax) >= padY) {
            r.dx = ((s16(u(B_X + r.bx)) shr 5) + u(r.si)) and 0xffff
            if (s16(r.dx) >= d(Game.PAD_LEFT + r.di)) {
                r.dx = (r.dx - 1) and 0xffff
                if (s16(r.dx) <= d(Game.PAD_RIGHT + r.di)) {
                    r.ax = ((u(Game.PAD_Y + r.di) - u(r.si + 2)) shl 5) and 0xffff
                    setD(B_Y + r.bx, r.ax)
                    bonusState(u(r.si + 8), r)
                    return
                }
            }
        }
        r.di = (r.di + 2) and 0xffff
    }
}

/** 113b:2c97 - landed: lies on the pad for 0x230 frames. */
private fun Game.b2c97Landed(r: Regs) {
    r.si = u(B_DESC + r.bx)
    r.ax = u(r.si + 0x0a)
    setD(B_STATE + r.bx, r.ax)
    setD(B_VX + r.bx, 0x230)
}

/** 113b:2ca9 - lying; a copter touching it collects it. */
private fun Game.b2ca9Lying(r: Regs) {
    val expired = (u(B_VX + r.bx) - 1) and 0xffff
    setD(B_VX + r.bx, expired)
    if (expired == 0) { setD(B_SPRITE + r.bx, 0xffff); return }
    r.si = u(B_DESC + r.bx)
    if (!copterTouchesBonus(r)) return
    r.ax = u(r.si + 0x12)
    when (r.ax) {
        0 -> { // energy
            r.ax = (u(r.si + 0x0e) + u(Game.V_ENERGY)) and 0xffff
            if (r.ax > 0x5a3b) r.ax = 0x5a3b
            setD(Game.V_ENERGY, r.ax)
        }
        1 -> { // lives, at most 99
            r.ax = u(r.si + 0x0e)
            var al = ((r.ax and 0xff) + d8(0x263c)) and 0xff
            if (al > 0x63) al = 0x63
            r.ax = (r.ax and 0xff00) or al
            setD8(0x263c, al)
        }
        else -> { // score multiplier, limited by the difficulty
            r.ax = u(0x263d)
            r.di = u(Game.V_DIFFICULTY) shl 1
            if (r.ax < u(0x2628 + r.di)) {
                val al = ((r.ax and 0xff) + 1) and 0xff
                r.ax = (r.ax and 0xff00) or al
                setD8(0x263d, al)
            }
        }
    }
    setD(B_SPRITE + r.bx, 0xffff)
}

/** 113b:2207 - is a copter touching bonus item BX (box from descriptor SI)? DI = player*2 or 0xffff. */
fun Game.copterTouchesBonus(r: Regs): Boolean {
    r.di = 0
    while (true) {
        val si = r.si
        var ok = true
        r.ax = ((u(si + 2) shl 5) + u(B_Y + r.bx)) and 0xffff
        if (s16(r.ax) < d(Game.P_YF + r.di)) ok = false
        if (ok) {
            r.ax = ((((u(si + 2) - 6) shl 5) + u(B_Y + r.bx)) - 0x280) and 0xffff
            if (s16(r.ax) > d(Game.P_YF + r.di)) ok = false
        }
        if (ok) {
            r.ax = ((((u(si) + 3) shl 5) + u(B_X + r.bx)) - 0xa0) and 0xffff
            if (s16(r.ax) < d(Game.P_XF + r.di)) ok = false
        }
        if (ok) {
            r.ax = ((((u(si) - 3) shl 5) + u(B_X + r.bx)) - 0x340) and 0xffff
            if (s16(r.ax) <= d(Game.P_XF + r.di)) return true
        }
        r.di = (r.di + 2) and 0xffff
        if (r.di != u(0x2634)) break
    }
    r.di = 0xffff
    return false
}
