package ugh.core.game

import ugh.core.game.Game.Companion.s16

/*
 * Passengers (level list B) - port of 113b:1486 .. 113b:2276.
 *
 * Every passenger is a state machine. DGROUP:2a0d[i] holds the address of its current state handler in the
 * original code segment; the handlers come from the passenger's descriptor (DGROUP:29ad[i], e.g. 0x7720) and
 * jump to each other with JMP [SI+n], handing over registers. The port keeps the handler addresses as data
 * (so the memory stays identical to the original) and dispatches them to the Kotlin functions below, passing
 * the register file along like the original does.
 *
 * Arrays (index = passenger * 2):
 *   29ad descriptor   29cd start pad   29ed route pointer   2a0d state handler   2a2d counter / carrying player
 *   2a4d pickup pad   2a6d target pad  2a8d pickup pad*2    2aad target pad*2    2acd timer
 *   2aed x (1/32 px)  2b0d y (1/32 px) 2b2d x px  2b6d y px  (2b4d/2b8d previous, used by the renderer)
 *   2bad animation index  2bcd animation delay  2bed sprite  2c2d bubble sprite  2c6d vertical speed
 *   2c8d bonus timer
 * Pads: 2999[pad] = passenger waiting there or -1, 2985[pad] = pad number shown.
 * Players: 27fc carried passenger type (0 = none), 2804 target pad number shown, 2808 fare, 280c minimum fare.
 */

private const val P_DESC = 0x29ad
private const val P_ROUTE = 0x29ed
private const val P_STATE = 0x2a0d
private const val P_COUNT = 0x2a2d
private const val P_FROM = 0x2a4d
private const val P_TO = 0x2a6d
private const val P_FROM2 = 0x2a8d
private const val P_TO2 = 0x2aad
private const val P_TIMER = 0x2acd
private const val P_XF = 0x2aed
private const val P_YF = 0x2b0d
private const val P_X = 0x2b2d
private const val P_Y = 0x2b6d
private const val P_ANIM = 0x2bad
private const val P_DELAY = 0x2bcd
private const val P_SPRITE = 0x2bed
private const val P_BUBBLE = 0x2c2d
private const val P_VY = 0x2c6d
private const val P_BONUS = 0x2c8d

private const val PAD_W3 = 0x2949
private const val PAD_W4 = 0x295d
private const val PAD_NUMBER = 0x2985
private const val PAD_WAITING = 0x2999

private const val PL_CARRY = 0x27fc
private const val PL_TARGET = 0x2804
private const val PL_FARE = 0x2808
private const val PL_FARE_MIN = 0x280c

private const val SCORE_LO = 0x261e
private const val SCORE_HI = 0x2620
private const val MULTIPLIER = 0x263d
private const val MULTIPLIER_LIMITS = 0x2628
private const val PLAYERS = 0x2634
private const val PLAYERS2 = 0x2636

/** Descriptor of the bonus items dropped for quick deliveries (DGROUP:7a38). */
private const val BONUS_DESCRIPTOR = 0x7a38

/** ADLX sound effects used by the passengers. */
private const val SFX_PAID = 0x4274
private const val SFX_DROP = 0x4632

/** 113b:1486 - runs the state handler of every passenger. */
fun Game.passengersUpdate(r: Regs = Regs()) {
    r.bx = 0
    while (u(P_DESC + r.bx) != 0xffff) {
        passengerState(u(P_STATE + r.bx), r)
        r.bx = (r.bx + 2) and 0xffff
    }
}

/** Dispatches a passenger state handler address of the original to its port. */
fun Game.passengerState(addr: Int, r: Regs) {
    when (addr) {
        0x149c -> p149cNextStop(r)
        0x1509 -> p1509Arriving(r)
        0x153b -> p153bAppear(r)
        0x1582 -> p1582Appearing(r)
        0x15b4 -> p15b4StartWaiting(r)
        0x15d7 -> p15d7Waiting(r)
        0x16f6 -> p16f6StartCalling(r)
        0x172a -> p172aCalling(r)
        0x17e6 -> p17e6StartImpatient(r)
        0x180a -> p180aImpatient(r)
        0x18c8 -> p18c8StartBoarding(r)
        0x18e6 -> p18e6Boarding(r)
        0x19e0 -> p19e0Board(r, switchDescriptor = true)
        0x19fb -> p19e0Board(r, switchDescriptor = false)
        0x1a42 -> p1a42Riding(r)
        0x1a7e -> p1a7ePaid(r)
        0x1b29 -> p1b29WalkingAway(r)
        0x1bbe -> p1bbeStartEntering(r)
        0x1bd6 -> p1bd6Entering(r)
        0x1c0e, 0x2195 -> {}
        0x1c0f -> p1c0fStartStanding(r)
        0x1c27 -> p1c27Standing(r)
        0x1c48 -> p1c48Grabbed(r)
        0x1c6b -> p1c6bHanging(r)
        0x1c81 -> p1c81Dropped(r)
        0x1cee -> p1ceeFalling(r)
        0x1da8 -> p1da8StartSplash(r)
        0x1dd5 -> p1dd5Splash(r, advance = false)
        0x1de2 -> p1dd5Splash(r, advance = true)
        0x1e9c -> p1e9cStartSinking(r)
        0x1ec0 -> p1ec0Sinking(r)
        0x1f24 -> p1f24StartSwimming(r)
        0x1f43 -> p1f43Swimming(r)
        0x1fe2 -> p1fe2SwimCalling(r)
        0x2068 -> p2068SwimWaving(r)
        0x20c1 -> p20c1SwimBoarding(r)
        else -> error("passenger state handler %04x not ported (%s)".format(addr, r))
    }
}

/** JMP word ptr [SI+off] */
private fun Game.jumpVia(r: Regs, off: Int) = passengerState(u(r.si + off), r)

// ---------------------------------------------------------------- helpers

/** Common animation step: next entry of the frame table at descriptor+[table], restarting at 0xffff. */
private fun Game.nextFrame(r: Regs, table: Int) {
    while (true) {
        r.di = (u(r.si + table) + u(P_ANIM + r.bx)) and 0xffff
        r.ax = u(r.di)
        if (r.ax != 0xffff) break
        setD(P_ANIM + r.bx, 0)
    }
    setD(P_SPRITE + r.bx, r.ax)
}

/** DEC word [BX+off]; returns true when it became zero. */
private fun Game.decZero(off: Int): Boolean {
    val v = (u(off) - 1) and 0xffff
    setD(off, v)
    return v == 0
}

/** Advances the animation delay; on expiry reloads it from descriptor+3e and steps the frame index. */
private fun Game.animTick(r: Regs): Boolean {
    if (!decZero(P_DELAY + r.bx)) return false
    r.ax = u(r.si + 0x3e)
    setD(P_DELAY + r.bx, r.ax)
    addD(P_ANIM + r.bx, 2)
    return true
}

/** Resets the animation of a new state. */
private fun Game.animReset(bx: Int) {
    setD(P_ANIM + bx, 0xfffe)
    setD(P_DELAY + bx, 1)
}

/** If the passenger's feet reached the water: switch to the descriptor's water set (+48). */
private fun Game.fellIntoWater(r: Regs): Boolean {
    r.ax = ((s16(u(r.si + 2)) shr 1) + u(P_Y + r.bx) - u(Game.V_WATER_ROW)) and 0xffff
    if (s16(r.ax) < 0) return false
    r.ax = ((u(Game.V_WATER_ROW) + u(r.si + 2)) shl 5) and 0xffff
    setD(P_YF + r.bx, r.ax)
    r.di = u(P_FROM2 + r.bx)
    r.ax = 0xffff
    setD(PAD_WAITING + r.di, r.ax)
    setD(P_BUBBLE + r.bx, r.ax)
    switchToWaterSet(r)
    return true
}

private fun Game.switchToWaterSet(r: Regs) {
    r.si = u(r.si + 0x48)
    setD(P_DESC + r.bx, r.si)
    jumpVia(r, 0x08)
}

/** Knocked down by a copter flying through: 2276 finds a player whose copter is not landed. */
private fun Game.hitByCopter(r: Regs): Boolean {
    if (!touchesPlayer(r)) return false
    if (r.ax != 0xffff) return false
    switchToWaterSet(r)
    return true
}

/**
 * 113b:2276 - is a copter touching the passenger? Uses the passenger's box from its descriptor
 * (+0 x offset, +2 y offset, +4 half width, +6 half height). Returns true with DI = player*2 and
 * AX = that player's landed pad; false with DI = 0xffff.
 */
fun Game.touchesPlayer(r: Regs): Boolean {
    r.di = 0
    while (true) {
        val si = r.si
        var ok = true
        r.ax = ((u(si + 2) shl 5) + u(P_YF + r.bx)) and 0xffff
        if (s16(r.ax) < d(Game.P_YF + r.di)) ok = false
        if (ok) {
            r.ax = ((((u(si + 2) - u(si + 6) - u(si + 6)) shl 5) + u(P_YF + r.bx)) - 0x280) and 0xffff
            if (s16(r.ax) > d(Game.P_YF + r.di)) ok = false
        }
        if (ok) {
            r.ax = ((((u(si) + u(si + 4)) shl 5) + u(P_XF + r.bx)) - 0xa0) and 0xffff
            if (s16(r.ax) < d(Game.P_XF + r.di)) ok = false
        }
        if (ok) {
            r.ax = ((((u(si) - u(si + 4)) shl 5) + u(P_XF + r.bx)) - 0x340) and 0xffff
            if (s16(r.ax) <= d(Game.P_XF + r.di)) {
                r.ax = u(Game.P_LANDED + r.di)
                return true
            }
        }
        r.di = (r.di + 2) and 0xffff
        if (r.di != u(PLAYERS)) break
    }
    r.di = 0xffff
    return false
}

/** Loop "is any player landed on pad AX (and, if [emptyHanded], carrying nobody)?"; leaves DI at the player. */
private fun Game.playerOnPad(r: Regs, pad: Int): Int {
    r.di = 0
    while (true) {
        if (pad == u(Game.P_LANDED + r.di)) return r.di
        r.di = (r.di + 2) and 0xffff
        if (r.di == u(PLAYERS2)) return -1
    }
}

// ---------------------------------------------------------------- states of the walking passenger

/** 113b:149c - takes the next stop of the route, or leaves the level when the route is finished. */
private fun Game.p149cNextStop(r: Regs) {
    r.di = u(P_ROUTE + r.bx)
    r.ax = u(r.di + 4)
    if (r.ax == 0xffff) {
        val left = (d8(0x28f1) - 1) and 0xff
        if (left and 0x80 == 0) {
            setD8(0x28f1, left)
            if (left == 0) { // last passenger done: end of level
                r.ax = 0xfffe
                setD(Game.V_FADE_STEP, r.ax)
                addD(Game.V_FADE, r.ax)
                setD8(0x27cf, r.ax and 0xff)
            }
        }
        setD(P_SPRITE + r.bx, 0xffff)
        r.si = u(P_DESC + r.bx)
        r.ax = u(r.si + 0x2c)
        setD(P_STATE + r.bx, r.ax)
        passengerState(r.ax, r)
        return
    }
    setD(P_TO + r.bx, r.ax)
    setD(P_TO2 + r.bx, r.ax shl 1)
    r.ax = u(r.di)
    setD(P_FROM + r.bx, r.ax)
    setD(P_FROM2 + r.bx, r.ax shl 1)
    r.ax = times3half(u(r.di + 2))
    setD(P_TIMER + r.bx, r.ax)
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x0a)
    setD(P_STATE + r.bx, r.ax)
}

/** 113b:1509 - hidden, waits for the timer and a free pickup pad. */
private fun Game.p1509Arriving(r: Regs) {
    setD(P_SPRITE + r.bx, 0xffff)
    r.ax = u(P_TIMER + r.bx)
    if (r.ax != 0) {
        r.ax = (r.ax - 1) and 0xffff
        setD(P_TIMER + r.bx, r.ax)
        if (r.ax != 0) return
    }
    r.di = u(P_FROM2 + r.bx)
    if (u(PAD_WAITING + r.di) != 0xffff) return
    r.si = u(P_DESC + r.bx)
    jumpVia(r, 0x0c)
}

/** 113b:153b - appears at the door of the pickup pad. */
private fun Game.p153bAppear(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x0e)
    setD(P_STATE + r.bx, r.ax)
    r.di = u(P_FROM2 + r.bx)
    setD(PAD_WAITING + r.di, r.bx)
    r.ax = (-u(r.si + 2) + u(Game.PAD_Y + r.di)) and 0xffff
    setD(P_Y + r.bx, r.ax)
    r.ax = (r.ax shl 5) and 0xffff
    setD(P_YF + r.bx, r.ax)
    r.ax = (-u(r.si) + u(PAD_W3 + r.di)) and 0xffff
    setD(P_X + r.bx, r.ax)
    r.ax = (r.ax shl 5) and 0xffff
    setD(P_XF + r.bx, r.ax)
    animReset(r.bx)
}

/** 113b:1582 - door animation (descriptor+36), then +10. */
private fun Game.p1582Appearing(r: Regs) {
    if (!decZero(P_DELAY + r.bx)) return
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x3e)
    setD(P_DELAY + r.bx, r.ax)
    addD(P_ANIM + r.bx, 2)
    r.di = (u(r.si + 0x36) + u(P_ANIM + r.bx)) and 0xffff
    r.ax = u(r.di)
    if (r.ax == 0xffff) { jumpVia(r, 0x10); return }
    setD(P_SPRITE + r.bx, r.ax)
}

/** 113b:15b4 */
private fun Game.p15b4StartWaiting(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x12)
    setD(P_STATE + r.bx, r.ax)
    animReset(r.bx)
    r.ax = 0xffff
    setD(P_COUNT + r.bx, r.ax)
    setD(P_BUBBLE + r.bx, r.ax)
}

/** 113b:15d7 - walks to the waiting spot of the pad (pad w4) and waits for a copter. */
private fun Game.p15d7Waiting(r: Regs) {
    r.di = u(P_FROM2 + r.bx)
    r.si = u(P_DESC + r.bx)
    if (fellIntoWater(r)) return
    if (decZero(P_DELAY + r.bx)) {
        r.ax = u(r.si + 0x3e)
        setD(P_DELAY + r.bx, r.ax)
        addD(P_ANIM + r.bx, 2)
        r.ax = (u(P_X + r.bx) + u(r.si)) and 0xffff
        val target = d(PAD_W4 + r.di)
        if (r.ax == u(PAD_W4 + r.di)) {
            if (u(P_COUNT + r.bx) != 1) setD(P_ANIM + r.bx, 0)
            while (true) {
                r.di = (u(r.si + 0x2e) + u(P_ANIM + r.bx)) and 0xffff
                r.ax = u(r.di)
                if (r.ax != 0xffff) break
                setD(P_ANIM + r.bx, 0)
            }
            setD(P_SPRITE + r.bx, r.ax)
            setD(P_COUNT + r.bx, 1)
        } else {
            setD(P_COUNT + r.bx, 0)
            if (s16(r.ax) < target) { nextFrame(r, 0x34); addD(P_XF + r.bx, 0x20) }
            else { nextFrame(r, 0x32); addD(P_XF + r.bx, -0x20) }
        }
    }
    if (hitByCopter(r)) return
    r.ax = u(P_FROM + r.bx)
    r.di = 0
    while (true) {
        if (r.ax == u(Game.P_LANDED + r.di) && u(PL_CARRY + r.di) == 0) { jumpVia(r, 0x14); return }
        r.di = (r.di + 2) and 0xffff
        if (r.di == u(PLAYERS2)) return
    }
}

/** 113b:16f6 - shows the destination bubble. */
private fun Game.p16f6StartCalling(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x16)
    setD(P_STATE + r.bx, r.ax)
    animReset(r.bx)
    r.ax = (u(P_TO + r.bx) + 0x10c) and 0xffff
    if (r.ax >= 0x112) r.ax = 0x111
    setD(P_BUBBLE + r.bx, r.ax)
    setD(P_COUNT + r.bx, 0x8c)
}

/** 113b:172a - a copter is on the pad: calls it for a while. */
private fun Game.p172aCalling(r: Regs) {
    r.si = u(P_DESC + r.bx)
    if (fellIntoWater(r)) return
    if (hitByCopter(r)) return
    r.ax = u(P_FROM + r.bx)
    val p = playerOnPad(r, r.ax)
    if (p < 0 || u(PL_CARRY + r.di) != 0) { jumpVia(r, 0x18); return }
    if (animTick(r)) nextFrame(r, 0x30)
    if (decZero(P_COUNT + r.bx)) jumpVia(r, 0x1c)
}

/** 113b:17e6 */
private fun Game.p17e6StartImpatient(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x1a)
    setD(P_STATE + r.bx, r.ax)
    animReset(r.bx)
    setD(P_BUBBLE + r.bx, 0x112)
    setD(P_COUNT + r.bx, 0x8c)
}

/** 113b:180a - the copter left without it: waves impatiently, then waits again or calls the next copter. */
private fun Game.p180aImpatient(r: Regs) {
    r.si = u(P_DESC + r.bx)
    if (fellIntoWater(r)) return
    if (hitByCopter(r)) return
    if (animTick(r)) nextFrame(r, 0x30)
    if (!decZero(P_COUNT + r.bx)) return
    r.di = u(P_FROM2 + r.bx)
    r.ax = u(P_FROM + r.bx)
    r.di = 0
    while (true) {
        if (r.ax == u(Game.P_LANDED + r.di) && u(PL_CARRY + r.di) == 0) { jumpVia(r, 0x14); return }
        r.di = (r.di + 2) and 0xffff
        if (r.di == u(PLAYERS2)) break
    }
    jumpVia(r, 0x10)
}

/** 113b:18c8 */
private fun Game.p18c8StartBoarding(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x1e)
    setD(P_STATE + r.bx, r.ax)
    animReset(r.bx)
    setD(P_BUBBLE + r.bx, 0xffff)
}

/** Walks towards the copter of player DI; boards (descriptor+20) when level with it. [cx] = frame advanced. */
private fun Game.walkToCopter(r: Regs) {
    r.cx = 0
    if (decZero(P_DELAY + r.bx)) {
        r.ax = u(r.si + 0x3e)
        setD(P_DELAY + r.bx, r.ax)
        addD(P_ANIM + r.bx, 2)
        r.cx = 1
    }
    r.ax = (u(P_X + r.bx) + u(r.si) - 0x10) and 0xffff
    val copterX = d(Game.P_X + r.di)
    when {
        s16(r.ax) > copterX -> { nextFrame(r, 0x32); if (r.cx != 0) addD(P_XF + r.bx, -0x20) }
        s16(r.ax) < copterX -> { nextFrame(r, 0x34); if (r.cx != 0) addD(P_XF + r.bx, 0x20) }
        else -> jumpVia(r, 0x20)
    }
}

/** 113b:18e6 - walks to the landed copter. */
private fun Game.p18e6Boarding(r: Regs) {
    r.si = u(P_DESC + r.bx)
    if (fellIntoWater(r)) return
    if (hitByCopter(r)) return
    r.ax = u(P_FROM + r.bx)
    if (playerOnPad(r, r.ax) < 0) { jumpVia(r, 0x18); return }
    walkToCopter(r)
}

/** 113b:19e0 / 19fb - boards the copter of player DI: fare from the descriptor, target pad shown. */
private fun Game.p19e0Board(r: Regs, switchDescriptor: Boolean) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x40)
    setD(PL_FARE + r.di, r.ax)
    r.ax = u(r.si + 0x42)
    setD(PL_FARE_MIN + r.di, r.ax)
    if (switchDescriptor) {
        r.si = u(r.si + 0x48)
        setD(P_DESC + r.bx, r.si)
    }
    // 113b:1a0d
    r.ax = u(r.si + 0x22)
    setD(P_STATE + r.bx, r.ax)
    r.ax = u(r.si + 0x46)
    setD(PL_CARRY + r.di, r.ax)
    r.si = u(P_TO2 + r.bx)
    r.ax = u(PAD_NUMBER + r.si)
    setD(PL_TARGET + r.di, r.ax)
    setD(P_COUNT + r.bx, r.di)
    setD(P_SPRITE + r.bx, 0xffff)
    setD(P_BONUS + r.bx, 0xc8)
    r.di = u(P_FROM2 + r.bx)
    setD(PAD_WAITING + r.di, 0xffff)
}

/** 113b:1a42 - riding: the fare drops to its minimum, the bonus timer runs, until landing on the target pad. */
private fun Game.p1a42Riding(r: Regs) {
    r.di = u(P_COUNT + r.bx)
    r.ax = u(PL_FARE + r.di)
    if (r.ax > u(PL_FARE_MIN + r.di)) { r.ax = (r.ax - 1) and 0xffff; setD(PL_FARE + r.di, r.ax) }
    r.ax = u(P_BONUS + r.bx)
    if (s16(r.ax) > 0) { r.ax = (r.ax - 1) and 0xffff; setD(P_BONUS + r.bx, r.ax) }
    r.ax = u(P_TO + r.bx)
    if (r.ax == u(Game.P_LANDED + r.di)) {
        r.si = u(P_DESC + r.bx)
        jumpVia(r, 0x24)
    }
}

/** 113b:1a7e - delivered: pays fare * multiplier; a quick delivery drops a bonus item. */
private fun Game.p1a7ePaid(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x26)
    setD(P_STATE + r.bx, r.ax)
    r.ax = 0
    setD(PL_CARRY + r.di, 0)
    setD(PL_TARGET + r.di, 0)
    r.ax = ((0x10 - u(r.si)) shl 5) and 0xffff
    r.di = u(P_COUNT + r.bx)
    r.ax = (r.ax + u(Game.P_XF + r.di)) and 0xffff
    setD(P_XF + r.bx, r.ax)
    val product = u(PL_FARE + r.di).toLong() * u(MULTIPLIER)
    r.ax = (product and 0xffff).toInt()
    r.dx = ((product shr 16) and 0xffff).toInt()
    val lo = u(SCORE_LO) + r.ax
    setD(SCORE_LO, lo)
    setD(SCORE_HI, u(SCORE_HI) + r.dx + (lo shr 16))
    if (u(P_BONUS + r.bx) != 0) {
        val savedSi = r.si
        r.ax = u(MULTIPLIER)
        r.si = u(Game.V_DIFFICULTY) shl 1
        if (r.ax < u(MULTIPLIER_LIMITS + r.si)) {
            r.ax = (u(Game.P_XF + r.di) + 0x200) and 0xffff
            r.bp = (u(Game.P_YF + r.di) + 0x140) and 0xffff
            r.cx = 0
            r.dx = 0
            r.si = BONUS_DESCRIPTOR
            bonusSpawn(r)
            playEffect(r, SFX_PAID)
        }
        r.si = savedSi
    }
    r.di = u(P_TO2 + r.bx)
    r.ax = ((-u(r.si + 2) + u(Game.PAD_Y + r.di)) shl 5) and 0xffff
    setD(P_YF + r.bx, r.ax)
    setD(P_ANIM + r.bx, 0xfffe)
    r.ax = 1
    setD(P_DELAY + r.bx, 1)
    setD8(0x27cd, 1)
}

/** 113b:1b29 - walks from the copter to the house of the target pad (pad w3). */
private fun Game.p1b29WalkingAway(r: Regs) {
    r.di = u(P_TO2 + r.bx)
    r.si = u(P_DESC + r.bx)
    if (!decZero(P_DELAY + r.bx)) return
    r.ax = u(r.si + 0x3e)
    setD(P_DELAY + r.bx, r.ax)
    addD(P_ANIM + r.bx, 2)
    r.ax = (u(P_X + r.bx) + u(r.si)) and 0xffff
    val door = d(PAD_W3 + r.di)
    if (r.ax == u(PAD_W3 + r.di)) { jumpVia(r, 0x28); return }
    if (s16(r.ax) < door) { nextFrame(r, 0x34); addD(P_XF + r.bx, 0x20) }
    else { nextFrame(r, 0x32); addD(P_XF + r.bx, -0x20) }
    r.di = u(P_TO2 + r.bx)
    r.cx = 0
    r.ax = ((s16(u(P_XF + r.bx)) shr 5) + u(r.si)) and 0xffff
    if (r.ax == u(PAD_W3 + r.di)) jumpVia(r, 0x28)
}

/** 113b:1bbe */
private fun Game.p1bbeStartEntering(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x2a)
    setD(P_STATE + r.bx, r.ax)
    animReset(r.bx)
}

/** 113b:1bd6 - door animation (descriptor+38); then the next stop of the route. */
private fun Game.p1bd6Entering(r: Regs) {
    if (!decZero(P_DELAY + r.bx)) return
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x3e)
    setD(P_DELAY + r.bx, r.ax)
    addD(P_ANIM + r.bx, 2)
    r.di = (u(r.si + 0x38) + u(P_ANIM + r.bx)) and 0xffff
    r.ax = u(r.di)
    setD(P_SPRITE + r.bx, r.ax)
    if (u(r.di + 2) != 0xffff) return
    addD(P_ROUTE + r.bx, 4)
    jumpVia(r, 0x08)
}

// ---------------------------------------------------------------- the standing passenger (type 78dc)

/** 113b:1c0f */
private fun Game.p1c0fStartStanding(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x12)
    setD(P_STATE + r.bx, r.ax)
    animReset(r.bx)
}

/** 113b:1c27 - waits to be grabbed by a copter that carries nobody. */
private fun Game.p1c27Standing(r: Regs) {
    r.si = u(P_DESC + r.bx)
    if (touchesPlayer(r) && u(PL_CARRY + r.di) == 0) { jumpVia(r, 0x20); return }
    setD(P_SPRITE + r.bx, 0x220)
}

/** 113b:1c48 */
private fun Game.p1c48Grabbed(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x22)
    setD(P_STATE + r.bx, r.ax)
    r.ax = u(r.si + 0x46)
    setD(PL_CARRY + r.di, r.ax)
    setD(PL_TARGET + r.di, 7)
    setD(P_COUNT + r.bx, r.di)
    setD(P_SPRITE + r.bx, 0xffff)
}

/** 113b:1c6b - hangs on the copter until its pilot presses fire. */
private fun Game.p1c6bHanging(r: Regs) {
    r.di = u(P_COUNT + r.bx)
    if (d8(0x279c + r.di) and 0x80 == 0) return
    r.si = u(P_DESC + r.bx)
    jumpVia(r, 0x24)
}

/** 113b:1c81 - released: falls with the copter's momentum. */
private fun Game.p1c81Dropped(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = 0
    setD(PL_CARRY + r.di, 0)
    setD(PL_TARGET + r.di, 0)
    r.ax = (s16(u(Game.P_VX + r.di)) shr 5) and 0xffff
    setD(P_TIMER + r.bx, r.ax)
    r.ax = (s16(u(Game.P_VY + r.di)) shr 5) and 0xffff
    setD(P_VY + r.bx, r.ax)
    r.ax = (u(Game.P_XF + r.di) + 0x200) and 0xffff
    r.bp = (u(Game.P_YF + r.di) + 0x140) and 0xffff
    r.di = (u(r.si) shl 5) and 0xffff
    r.ax = (r.ax - r.di) and 0xffff
    setD(P_XF + r.bx, r.ax)
    r.di = (u(r.si + 2) shl 4) and 0xffff
    r.bp = (r.bp - r.di) and 0xffff
    setD(P_YF + r.bx, r.bp)
    setD(P_SPRITE + r.bx, 0x221)
    r.ax = u(r.si + 0x26)
    setD(P_STATE + r.bx, r.ax)
    playEffect(r, SFX_DROP)
}

/** 113b:1cee - falling; lands on a pad (+10) or disappears off screen (+2c). */
private fun Game.p1ceeFalling(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = (u(P_TIMER + r.bx) + u(P_XF + r.bx)) and 0xffff
    if (s16(r.ax) <= -0x200 || s16(r.ax) >= 0x2800) { gone(r); return }
    setD(P_XF + r.bx, r.ax)
    r.ax = (u(P_VY + r.bx) + 2) and 0xffff
    setD(P_VY + r.bx, r.ax)
    if (s16(r.ax) < 0) { addD(P_YF + r.bx, r.ax); return }
    r.cx = u(P_YF + r.bx)
    r.ax = (r.ax + r.cx) and 0xffff
    if (s16(r.ax) >= 0x1800) { gone(r); return }
    setD(P_YF + r.bx, r.ax)
    r.ax = ((s16(r.ax) shr 5) + u(r.si + 2)) and 0xffff
    r.cx = ((s16(r.cx) shr 5) + u(r.si + 2)) and 0xffff
    r.di = 0
    while (d(Game.PAD_LEFT + r.di) >= 0) {
        val padY = d(Game.PAD_Y + r.di)
        if (s16(r.cx) <= padY && s16(r.ax) >= padY) {
            r.dx = ((s16(u(P_XF + r.bx)) shr 5) + u(r.si)) and 0xffff
            if (s16(r.dx) >= d(Game.PAD_LEFT + r.di) && s16(r.dx) <= d(Game.PAD_RIGHT + r.di)) {
                r.ax = ((u(Game.PAD_Y + r.di) - u(r.si + 2)) shl 5) and 0xffff
                setD(P_YF + r.bx, r.ax)
                jumpVia(r, 0x10)
                return
            }
        }
        r.di = (r.di + 2) and 0xffff
    }
}

private fun Game.gone(r: Regs) {
    setD(P_SPRITE + r.bx, 0xffff)
    r.ax = u(r.si + 0x2c)
    setD(P_STATE + r.bx, r.ax)
}

// ---------------------------------------------------------------- the water set (descriptors at +48)

/** 113b:1da8 */
private fun Game.p1da8StartSplash(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x0a)
    setD(P_STATE + r.bx, r.ax)
    r.di = u(P_FROM2 + r.bx)
    r.ax = 0xffff
    setD(PAD_WAITING + r.di, r.ax)
    setD(P_BUBBLE + r.bx, r.ax)
    animReset(r.bx)
    setD(P_VY + r.bx, 0)
}

/** 113b:1dd5 (1de2 = entry with the delay already expired) - splashes into the water and floats up. */
private fun Game.p1dd5Splash(r: Regs, advance: Boolean) {
    var step = advance
    if (!advance) {
        r.si = u(P_DESC + r.bx)
        step = decZero(P_DELAY + r.bx)
    }
    if (step) {
        r.ax = u(r.si + 0x3e)
        setD(P_DELAY + r.bx, r.ax)
        addD(P_ANIM + r.bx, 2)
    }
    nextFrame(r, 0x2e)
    r.cx = (u(P_Y + r.bx) - u(r.si + 2) - u(Game.V_WATER_ROW)) and 0xffff
    val below = s16(r.cx)
    val ch = when {
        below == 0 -> 0
        below < 0 -> (below shr 8).toByte().toInt()
        else -> { r.cx = (r.cx and 0xff) or 0x100; 1 }
    }
    r.ax = u(P_VY + r.bx)
    r.ax = if (ch < 0) (r.ax + 0x27) and 0xffff
    else if (s16(r.ax) > 0) (r.ax - 0x175) and 0xffff
    else (r.ax - 0x5c) and 0xffff
    r.ax = s16(r.ax).coerceIn(-0x1800, 0x1800) and 0xffff
    setD(P_VY + r.bx, r.ax)
    r.ax = ((s16(r.ax) shr 6) + u(P_YF + r.bx)) and 0xffff
    setD(P_YF + r.bx, r.ax)
    if (d(P_VY + r.bx) >= 0) return
    r.ax = ((s16(r.ax) shr 5) - u(r.si + 2)) and 0xffff
    if (s16(r.ax) > d(Game.V_WATER_ROW)) return
    r.ax = ((u(Game.V_WATER_ROW) - u(r.si + 2)) shl 5) and 0xffff
    setD(P_YF + r.bx, r.ax)
    jumpVia(r, 0x10)
}

/** 113b:1e9c */
private fun Game.p1e9cStartSinking(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x0e)
    setD(P_STATE + r.bx, r.ax)
    setD(P_BUBBLE + r.bx, 0xffff)
    animReset(r.bx)
    setD(P_VY + r.bx, 0)
}

/** 113b:1ec0 - drowns: sinks until off the bottom (+2c). */
private fun Game.p1ec0Sinking(r: Regs) {
    r.si = u(P_DESC + r.bx)
    animTick(r)
    nextFrame(r, 0x2e)
    r.ax = (u(P_VY + r.bx) + 0x27) and 0xffff
    setD(P_VY + r.bx, r.ax)
    r.ax = ((s16(r.ax) shr 6) + u(P_YF + r.bx)) and 0xffff
    if (r.ax < 0x1800) { setD(P_YF + r.bx, r.ax); return }
    r.ax = u(r.si + 0x2c)
    setD(P_STATE + r.bx, r.ax)
    setD(P_SPRITE + r.bx, 0xffff)
}

/** 113b:1f24 */
private fun Game.p1f24StartSwimming(r: Regs) {
    r.si = u(P_DESC + r.bx)
    r.ax = u(r.si + 0x12)
    setD(P_STATE + r.bx, r.ax)
    animReset(r.bx)
    r.ax = u(r.si + 0x44)
    setD(P_TIMER + r.bx, r.ax)
}

/** Keeps a swimmer on the water surface. */
private fun Game.floatOnSurface(r: Regs) {
    r.ax = u(Game.V_WATER_ROW)
    if (r.ax != u(P_Y + r.bx)) {
        r.ax = ((r.ax - u(r.si + 2)) shl 5) and 0xffff
        setD(P_YF + r.bx, r.ax)
    }
}

/** Is copter [di] landed on the water (not moving vertically handled by the caller)? */
private fun Game.onWater(di: Int) = (u(Game.P_Y + di) - u(Game.V_WATER_ROW) + 0x12) and 0xffff == 0

/** 113b:1f43 - swims; a copter floating nearby (+14) rescues it, otherwise it drowns when the timer ends. */
private fun Game.p1f43Swimming(r: Regs) {
    r.si = u(P_DESC + r.bx)
    animTick(r)
    nextFrame(r, 0x30)
    floatOnSurface(r)
    if (r.si != 0x77fe) {
        r.di = 0
        while (true) {
            if (u(Game.P_VY + r.di) == 0 && u(PL_CARRY + r.di) == 0) {
                r.cx = (u(Game.P_Y + r.di) - u(Game.V_WATER_ROW) + 0x12) and 0xffff
                if (r.cx == 0) { jumpVia(r, 0x14); return }
            }
            r.di = (r.di + 2) and 0xffff
            if (r.di >= 4) break
            if (u(PLAYERS) != 2) break
        }
    }
    if (decZero(P_TIMER + r.bx)) jumpVia(r, 0x0c)
}

/** 113b:1fe2 - a copter is on the water: waves (+1c when done), or back to swimming (+18). */
private fun Game.p1fe2SwimCalling(r: Regs) {
    r.si = u(P_DESC + r.bx)
    floatOnSurface(r)
    r.cx = (u(0x27dc) - u(Game.V_WATER_ROW) + 0x12) and 0xffff
    if (r.cx != 0) {
        var other = false
        if (u(PLAYERS) == 2) {
            r.cx = (u(0x27de) - u(Game.V_WATER_ROW) + 0x12) and 0xffff
            other = r.cx == 0
        }
        if (!other) { jumpVia(r, 0x18); return }
    }
    if (animTick(r)) nextFrame(r, 0x30)
    if (decZero(P_COUNT + r.bx)) jumpVia(r, 0x1c)
}

/** 113b:2068 - waves for a while (+0c when done). */
private fun Game.p2068SwimWaving(r: Regs) {
    r.si = u(P_DESC + r.bx)
    floatOnSurface(r)
    if (animTick(r)) nextFrame(r, 0x30)
    if (decZero(P_COUNT + r.bx)) jumpVia(r, 0x0c)
}

/** 113b:20c1 - swims to the copter on the water. */
private fun Game.p20c1SwimBoarding(r: Regs) {
    r.si = u(P_DESC + r.bx)
    floatOnSurface(r)
    r.di = 0
    while (true) {
        if (u(PL_CARRY + r.di) == 0) {
            r.cx = (u(Game.P_Y + r.di) - u(Game.V_WATER_ROW) + 0x12) and 0xffff
            if (r.cx == 0) { walkToCopter(r); return }
        }
        r.di = (r.di + 2) and 0xffff
        if (r.di >= 4) break
        if (u(PLAYERS) != 2) break
    }
    jumpVia(r, 0x18)
}
