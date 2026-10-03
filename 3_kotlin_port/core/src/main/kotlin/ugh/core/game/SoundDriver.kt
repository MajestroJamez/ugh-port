package ugh.core.game

/*
 * AdLib (OPL2) driver - port of the AdLib part of segment 1664 (the Sound Blaster part is never used: the
 * game initialises the library with the AdLib flag only). All driver and library state lives in segment
 * 1664 like in the original:
 *   0278 + v*0x14   voice v (0..8): +0 instrument (12 bytes), +0xc volume, +0xe level, +0x10 owner channel,
 *                   +0x12 frequency word (A0 / B0 register values)
 *   032c + v*0x14   the same for the music while an effect has taken the voice ("shadow" voice)
 *   n*0x2a          channel n (8..11 effects, 12 music), see SoundLibrary.kt; the driver sees it as a
 *                   structure at n*0x2a + 0x14: +4 repeats, +6/+8 start, +0x10/+0x12 position,
 *                   +0x1e/+0x20 time of the next event, +0x24 first voice, +0x26 tempo, +0x28 flags
 *   000c music tempo (PIT clocks per tick), 0236 busy, 023a/023c tick counter, 0246..024c volume fade,
 *   0256 music on, 0258 fade running, 025a music volume, 026a devices, 026c voices taken by effects,
 *   0274 channel being played, 10ef current effect structure, 10f5 operator offsets of the 9 voices,
 *   1122 timer handle of the tick, 1124 timer handle of the fade, 1126 AdLib installed,
 *   1127/1129 CPU speed calibration (delay loops; measured on the machine, not modelled).
 */

internal const val DRIVER_SEG = 0x1664

internal fun Game.dw(off: Int) = mem.read16(DRIVER_SEG, off and 0xffff)
internal fun Game.setDw(off: Int, v: Int) = mem.write16(DRIVER_SEG, off and 0xffff, v and 0xffff)
internal fun Game.db(off: Int) = mem.read8(DRIVER_SEG, off and 0xffff)
internal fun Game.setDb(off: Int, v: Int) = mem.write8(DRIVER_SEG, off and 0xffff, v and 0xff)

/** 32-bit word pair hi:lo below cHi:cLo (unsigned). */
internal fun below32(hi: Int, lo: Int, cHi: Int, cLo: Int) = hi < cHi || (hi == cHi && lo < cLo)

/** Adds [v] to the 32-bit value at [off] (lo word) / [off]+2 (hi word) of the driver segment. */
internal fun Game.addDw32(off: Int, v: Long) {
    val sum = ((dw(off + 2).toLong() shl 16) or dw(off).toLong()) + v
    setDw(off, sum.toInt()); setDw(off + 2, (sum ushr 16).toInt())
}

/** 1664:11f2 - one OPL register write (the delay loops around it only matter to real hardware). */
internal fun Game.opl(reg: Int, value: Int) = host.adlib(reg and 0xff, value and 0xff)

/** 1664:112b - clears the driver variables 000c..0493. */
internal fun Game.driverClear() { for (i in 0x000c until 0x0494) setDb(i, 0) }

/**
 * 1664:113f - measures the CPU speed with two timer interrupts (results in 1127/1129, used for the delay
 * loops only, so not modelled) and keeps the INT 8 vector it found.
 */
private fun Game.driverCalibrate() {
    setDw(0x10cb, mem.read16(0, 0x20)); setDw(0x10cd, mem.read16(0, 0x22))
}

/** Voice [v] taken by an effect, as seen from the music (the original compares bytes). */
private fun Game.effectVoice(v: Int) = dw(0x26c) and 0xf != 0 && v >= ((9 - db(0x26c)) and 0xff)

/** 1664:1211 - volume [vol] of voice [voice]: scales the carrier level (and the modulator in additive mode). */
internal fun Game.setVolume(voice: Int, vol: Int) {
    val v = voice and 0xff
    val dl = vol and 0xff
    if (v >= 9) return
    val si = 0x14 * v
    if (dw(0x256) and 1 != 0 && dw(0x274) >= 0xc && effectVoice(v)) { setDb(si + 0x338, dl); return }
    setDb(si + 0x284, dl)
    val cl = db(si + 0x286)
    val ch = db(0x10f5 + v)
    fun level(tl: Int): Int {
        var al = (0x3f - (tl and 0x3f)) and 0xff
        al = (al * cl) shr 8
        val ah = (al * dl) shr 8
        return ((0x3f - ah) and 0xff) or (tl and 0xc0)
    }
    opl(0x43 + ch, level(db(si + 0x27e)))
    if (dw(si + 0x282) and 1 != 0) opl(0x40 + ch, level(db(si + 0x279)))
}

/** 1664:1377 - instrument of voice [voice] from the 12 bytes at [seg]:[off]. */
internal fun Game.setInstrument(voice: Int, seg: Int, off: Int) {
    val v = voice and 0xff
    if (v >= 9) return
    val di = 0x14 * v
    val dest = if (dw(0x274) == 0xc && effectVoice(v)) 0x32c + di else 0x278 + di
    for (i in 0 until 12) setDb(dest + i, mem.read8(seg, (off + i) and 0xffff))
    if (dest != 0x278 + di) return
    fun b(i: Int) = mem.read8(seg, (off + i) and 0xffff)
    val ch = db(0x10f5 + v)
    opl(0x20 + ch, b(0))
    opl(0x40 + ch, if (b(10) and 1 != 0) b(1) or 0x3f else b(1))
    opl(0x60 + ch, b(2))
    opl(0x80 + ch, b(3))
    opl(0xe0 + ch, b(4))
    opl(0x23 + ch, b(5))
    opl(0x43 + ch, b(6) or 0x3f)
    opl(0x63 + ch, b(7))
    opl(0x83 + ch, b(8))
    opl(0x01, 0x20)
    opl(0xe3 + ch, b(9))
    opl(0xc0 + v, b(10))
    setDw(0x288 + di, dw(0x274))
    setVolume(v, db(0x284 + di))
}

/** 1664:14b1 - key off for voice [voice]. */
internal fun Game.keyOff(voice: Int) {
    val v = voice and 0xff
    if (v >= 9) return
    val bx = 0x14 * v
    val dl = db(0x28b + bx) and 0xdf
    setDb(0x28b + bx, dl)
    opl(0xb0 + v, dl)
}

/** 1664:1503 - frequency / key on word [value] (A0 low byte, B0 high byte) for voice [voice]. */
internal fun Game.setFreq(voice: Int, value: Int) {
    val v = voice and 0xff
    if (v >= 9) return
    setDw(0x28a + 0x14 * v, value)
    opl(0xa0 + v, value and 0xff)
    opl(0xb0 + v, (value shr 8) and 0xff)
}

/** 1664:1571 - level (velocity) [value] for voice [voice], then the volume again. */
internal fun Game.setLevel(voice: Int, value: Int) {
    val v = voice and 0xff
    val dl = value and 0xff
    if (v >= 9) return
    val si = 0x14 * v
    if (dw(0x274) >= 0xc && effectVoice(v)) { setDb(si + 0x33a, dl); return }
    setDb(si + 0x286, dl)
    setVolume(v, db(si + 0x284))
}

private fun Game.swapped(seg: Int, off: Int): Int {
    val w = mem.read16(seg, off and 0xffff)
    return ((w and 0xff) shl 8) or (w shr 8)
}

/**
 * 1664:15d3 - the music interpreter: events from [seg]:[off] up to the next pause. High nibble: 0 long pause,
 * 1 frequency, 2 key off, 3 level, 4 instrument, 5 nothing, 6 tempo, 7 meta event (0x77 end of track),
 * 8..f short pause. Voices taken by effects only get their shadow updated.
 */
private fun Game.musicEvents(seg: Int, off: Int) {
    var di = off
    fun next(): Int { val b = mem.read8(seg, di); di = (di + 1) and 0xffff; return b }
    while (true) {
        val b = next()
        val v = b and 0xf
        when (b shr 4) {
            0 -> {
                val dx = (((v shl 8) or next()) shl 7) and 0xffff
                addDw32(0x22a, dx.toLong()); setDw(0x21c, di); return
            }
            1 -> if (v < 9) {
                val ax = mem.read16(seg, di); di = (di + 2) and 0xffff
                if (v < ((9 - db(0x26c)) and 0xff)) setFreq(v, ax) else setDw(0x33e + 0x14 * v, ax)
            }
            2 -> if (v < 9) {
                if (v < ((9 - db(0x26c)) and 0xff)) keyOff(v) else setDw(0x33e + 0x14 * v, dw(0x33e + 0x14 * v) and 0xffdf)
            }
            3 -> { if (v < 9) setLevel(v, mem.read8(seg, di)); di = (di + 1) and 0xffff }
            4 -> if (v < 9) { setInstrument(v, seg, di); di = (di + 0xb) and 0xffff }
            5 -> {}
            6 -> {
                val bx = swapped(seg, di)
                setDw(0x000c, bx)
                timerSetPeriod(dw(0x1122), bx, 0)
                di = (di + 2) and 0xffff
            }
            7 -> {
                if (b == 0x77) { musicTrackEnd(); return }
                val ah = next()
                di = (di + if (v == 0) 2 else ah) and 0xffff
            }
            else -> { addDw32(0x22a, (b and 0x7f).toLong()); setDw(0x21c, di); return }
        }
    }
}

/** 1664:16f1 - end of the music track: again from the start while repeats are left (or forever). */
private fun Game.musicTrackEnd() {
    if (dw(0x234) and 2 == 0) {
        val left = (dw(0x210) - 1) and 0xffff
        setDw(0x210, left)
        if (left.toShort() <= 0) { setDw(0x234, dw(0x234) and 0xfffe); return }
    }
    setDw(0x21e, dw(0x214)); setDw(0x21c, dw(0x212))
    setDw(0x22a, dw(0x23a)); setDw(0x23e, dw(0x23a))
    setDw(0x22c, dw(0x23c)); setDw(0x240, dw(0x23c))
}

/**
 * 1664:176b - the effect interpreter for the structure in 10ef: like the music, but voices are relative to
 * the effect's first voice and pauses are scaled by its tempo (+0x26, 0x100 = the music's tick).
 */
private fun Game.effectEvents(seg: Int, off: Int) {
    var di = off
    fun next(): Int { val b = mem.read8(seg, di); di = (di + 1) and 0xffff; return b }
    while (true) {
        val b = next()
        val bx = dw(0x10ef)
        val al = ((b and 0xf) + db(bx + 0x24)) and 0xff
        when (b shr 4) {
            0 -> {
                val dx = ((((b and 0xf) shl 8) or next()) shl 7) and 0xffff
                val product = dx.toLong() * dw(bx + 0x26)
                addDw32(bx + 0x1e, product ushr 8)
                setDw(bx + 0x10, di); return
            }
            1 -> if (al < 9) { setFreq(al, mem.read16(seg, di)); di = (di + 2) and 0xffff }
            2 -> if (al < 9) keyOff(al)
            3 -> { if (al < 9) setLevel(al, mem.read8(seg, di)); di = (di + 1) and 0xffff }
            4 -> if (al <= 9) {
                // the original lets voice 9 through: setInstrument ignores it, but the owner word is written
                setDw(0x288 + 0x14 * al, dw(0x274))
                setInstrument(al, seg, di)
                di = (di + 0xb) and 0xffff
            }
            5 -> {}
            6 -> {
                if (dw(0x256) and 1 == 0 && dw(0x000c) == 0) {
                    val t = swapped(seg, di)
                    setDw(0x000c, t)
                    timerSetPeriod(dw(0x1122), t, 0)
                } else {
                    val w = mem.read16(seg, di)
                    val dividend = ((w and 0xff).toLong() shl 16) or ((w shr 8).toLong() shl 8)
                    val divisor = dw(0x000c)
                    check(divisor != 0 && dividend / divisor <= 0xffff) { "divide overflow in 1664:188a" }
                    setDw(bx + 0x26, (dividend / divisor).toInt())
                }
                di = (di + 2) and 0xffff
            }
            7 -> {
                if (b == 0x77) { effectTrackEnd(); return }
                val ah = next()
                di = (di + if (b and 0xf == 0) 2 else ah) and 0xffff
            }
            else -> {
                val product = (b and 0x7f).toLong() * dw(bx + 0x26)
                addDw32(bx + 0x1e, (product ushr 8) and 0xffff)
                setDw(bx + 0x10, di); return
            }
        }
    }
}

/** 1664:18bf - end of an effect: repeat, or stop its channel. */
private fun Game.effectTrackEnd() {
    val bx = dw(0x10ef)
    if (dw(bx + 0x28) and 2 == 0) {
        val left = (dw(bx + 4) - 1) and 0xffff
        setDw(bx + 4, left)
        if (left.toShort() <= 0) { libStop(dw(0x274)); return }
    }
    setDw(bx + 0x12, dw(bx + 8))
    setDw(bx + 0x10, dw(bx + 6))
}

/** 1664:1939 - the driver tick (timer slot, 0x1843 PIT clocks = 192 Hz, the music's tempo changes it). */
internal fun Game.driverTick() {
    addDw32(0x23a, 1)
    if (dw(0x236) and 1 != 0) return
    setDw(0x236, 1)
    if (dw(0x256) and 1 != 0 && !below32(dw(0x23c), dw(0x23a), dw(0x22c), dw(0x22a))) {
        setDw(0x10ef, 0x20c)
        setDw(0x274, 0xc)
        check(dw(0x25c) and 1 == 0) { "music streaming (1878:1943) not ported" }
        musicEvents(dw(0x21e), dw(0x21c))
    }
    if (dw(0x26c) and 0xf != 0) {
        var bx = 0x164
        setDw(0x274, 8)
        do {
            if (db(bx + 0x28) and 5 == 5 && !below32(dw(0x23c), dw(0x23a), dw(bx + 0x20), dw(bx + 0x1e))) {
                setDw(0x10ef, bx)
                effectEvents(dw(bx + 0x12), dw(bx + 0x10))
            }
            setDw(0x274, dw(0x274) + 1)
            bx += 0x2a
        } while (bx < 0x20c)
    }
    setDw(0x236, 0)
}

/** 1664:12e5 - volume fade step (timer slot, 10 Hz): from 024a to 024c in 0248 steps, then the slot ends. */
internal fun Game.fadeTick() {
    val dx = (dw(0x246) + 1) and 0xffff
    setDw(0x246, dx)
    val bx = dw(0x248)
    if (dx > bx) { timerRemove(dw(0x1124)); setDw(0x258, 0); return }
    val cx = dw(0x24a)
    val ax = (dw(0x24c) - cx) and 0xffff
    val q = (ax.toShort() * dx.toShort()) / bx.toShort()
    libSetMusicVolume((q + cx) and 0xffff)
}

/** 1664:1331 - stops the volume fade. */
internal fun Game.fadeStop() {
    if (dw(0x258) and 1 == 0) return
    timerRemove(dw(0x1124))
    setDw(0x258, 0)
}

/** 1664:134e - starts the volume fade timer (0x1d1eb PIT clocks = 10 Hz). */
internal fun Game.fadeStart() {
    if (dw(0x258) and 1 != 0) return
    setDw(0x1124, timerAdd(far(DRIVER_SEG, 0x12e5), 0xd1eb, 1))
    setDw(0x258, 1)
}

/** 1664:1a37 - silence: waveform select off/on and all operators to the lowest level. */
internal fun Game.adlibSilence() {
    opl(0x01, 0x00)
    opl(0x01, 0x20)
    for (v in 0 until 9) {
        val ch = db(0x10f5 + v)
        opl(0x40 + ch, 0x3f)
        opl(0x43 + ch, 0x3f)
    }
}

/** 1664:1a65 - AdLib off: tick slot and scheduler removed, silence. */
internal fun Game.adlibShutdown() {
    if (db(0x1126) != 0) timerRemove(dw(0x1122))
    timerUninstall()
    setDb(0x1126, 0)
    adlibSilence()
}

/** 1664:1a89 - AdLib detection (OPL timer 1 status) and set-up; 1 when a card answers. */
internal fun Game.adlibInit(): Int {
    if (db(0x1126) != 0) return 1
    setDb(0x1126, 1)
    driverCalibrate()
    opl(0x01, 0x00)
    opl(0x04, 0x60)
    opl(0x04, 0x80)
    if (!host.adlibPresent()) return 0                // status reads 0xFF without a card
    opl(0x02, 0xff)
    opl(0x04, 0x21)
    opl(0x04, 0x60)
    opl(0x04, 0x80)
    adlibSilence()
    opl(0x01, 0x20)
    timerInstall()
    setDw(0x1122, timerAdd(far(DRIVER_SEG, 0x1939), 0x1843, 0))
    return 1
}
