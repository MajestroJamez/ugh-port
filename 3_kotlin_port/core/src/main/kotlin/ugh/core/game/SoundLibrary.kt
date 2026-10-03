package ugh.core.game

/*
 * Sound library - port of the AdLib paths of segment 1878 (Borland C). Its own flags are in segment 2a37
 * (0: initialised, 2: shut down); everything else is in the driver segment 1664. Channel n (8..11 effects,
 * 12 music) is the structure at n*0x2a:
 *   +0x18 repeats, +0x1a/+0x1c start of the track, +0x1e data length, +0x24/+0x26 position,
 *   +0x30 volume, +0x32/+0x34 time of the next event, +0x36 voices used, +0x38 first voice, +0x3a tempo,
 *   +0x3c flags (1 playing, 2 repeat forever, 4 loaded).
 * Effects take voices from the top (voice 9 - 026c on), the music keeps writing their shadow voices.
 */

private const val LIB_SEG = 0x2a37

private fun ch(n: Int) = n * 0x2a

/** 1878:0a70 - low word of the big-endian 32-bit chunk length at [seg]:[off]+4. */
private fun Game.chunkLength(seg: Int, off: Int) =
    (mem.read8(seg, (off + 6) and 0xffff) shl 8) or mem.read8(seg, (off + 7) and 0xffff)

private fun Game.isTag(seg: Int, off: Int, tag: String) =
    tag.indices.all { mem.read8(seg, (off + it) and 0xffff) == tag[it].code }

/** 1878:0003 - library init with the device flags (the game passes 1 = AdLib); returns the devices found. */
fun Game.libInit(flags: Int): Int {
    var found = 0
    if (mem.read16(LIB_SEG, 0) != 0 && dw(0x26a) != 0) libShutdown()
    driverClear()
    check(flags and 2 == 0) { "Sound Blaster support not ported" }
    if (flags and 1 != 0 && adlibInit() != 0) found = found or 1
    setDw(0x26a, found)
    mem.write16(LIB_SEG, 0, 1)
    return found
}

/** 1878:008a - library shutdown. */
fun Game.libShutdown() {
    if (mem.read16(LIB_SEG, 2) != 0 && dw(0x26a) == 0) return
    setDw(0x236, 1)
    fadeStop()
    check(dw(0x26a) and 2 == 0) { "Sound Blaster support not ported" }
    if (dw(0x26a) and 1 != 0) adlibShutdown()
    setDw(0x26a, 0)
    mem.write16(LIB_SEG, 2, 1)
}

/** 1878:0691 - stops channel [n] and loads the ADLX block at [seg]:[off] into it with volume [vol]. */
private fun Game.libPrepare(n: Int, off: Int, seg: Int, vol: Int, repeats: Int): Int {
    if (n < 8) return 0xffff
    libStop(n)
    // the original tests the result of 0aa9 against 0, so a failed load (-1) still counts as success
    if (libLoad(n, off, seg, repeats) == 0) return 0xffff
    setDw(ch(n) + 0x30, vol)
    return 1
}

/** 1878:0aa9 - checks the ADLX/AHDR/IMAP/TRCK chunks at [seg]:[off] and sets channel [n] up for its track. */
private fun Game.libLoad(n: Int, off: Int, seg: Int, repeats: Int): Int {
    if (dw(0x26a) and 1 == 0) return 0xffff
    if (n.toShort() > 0xc || n.toShort() < 8) return 0xffff
    var p = off
    val adlx = p; p = (p + 8) and 0xffff
    if (!isTag(seg, adlx, "ADLX")) return 0xffff
    val ahdr = p; p = (p + 8) and 0xffff
    if (!isTag(seg, ahdr, "AHDR")) return 0xffff
    p = (p + chunkLength(seg, ahdr)) and 0xffff
    val imap = p; p = (p + 8) and 0xffff
    if (!isTag(seg, imap, "IMAP")) return 0xffff
    p = (p + chunkLength(seg, imap)) and 0xffff
    val trck = p; p = (p + 8) and 0xffff
    if (!isTag(seg, trck, "TRCK")) return 0xffff
    val voices = mem.read8(seg, (ahdr + 0xa) and 0xffff)
    if (n != 0xc && voices > ((9 - dw(0x26c)) and 0xffff)) return 0xffff
    if (n == 0xc) {
        setDw(0x268, seg); setDw(0x266, imap + 8)
        setDw(0x238, mem.read16(seg, (ahdr + 0xe) and 0xffff))
    }
    val c = ch(n)
    setDw(c + 0x3a, 0x100)
    setDw(c + 0x1e, chunkLength(seg, adlx) - 0x22)
    setDw(c + 0x36, voices)
    setDw(c + 0x26, seg); setDw(c + 0x24, p)
    setDw(c + 0x1c, seg); setDw(c + 0x1a, p)
    setDw(c + 0x3c, 4)
    if (repeats == 0) setDw(c + 0x3c, dw(c + 0x3c) or 2) else setDw(c + 0x18, repeats)
    return 1
}

/** 1878:0cfc - starts the loaded channel [n] with volume [vol]: an effect first takes its voices. */
private fun Game.libStart(n: Int, vol: Int): Int {
    val busy = dw(0x236)
    if (dw(0x26a) and 1 == 0) return 0xffff
    setDw(0x274, n)
    val v = if (vol and 0xff00 != 0) 0xff else vol
    val c = ch(n)
    if (n.toShort() > 0xc || n.toShort() < 8 || dw(c + 0x3c) and 4 == 0 || dw(c + 0x3c) and 1 != 0) return 0xffff
    setDw(0x236, 1)
    setDw(c + 0x30, v)
    if (n.toShort() < 0xc) {
        libStop(n)
        setDw(0x26c, dw(0x26c) + dw(c + 0x36))
        if (dw(0x26c) > 9) {
            setDw(0x26c, dw(0x26c) - dw(c + 0x36))
            return 0xffff                                  // the original leaves 0236 (busy) set here
        }
        setDw(c + 0x38, 9 - dw(0x26c))
    }
    var di = dw(c + 0x38)
    while (((dw(c + 0x38) + dw(c + 0x36)) and 0xffff) > di) {
        val t = 0x14 * di
        if (dw(0x22e) > di) {
            // the music's state of the voice goes to its shadow
            setDw(t + 0x33e, dw(t + 0x28a))
            for (i in 0 until 12) setDb(t + 0x32c + i, db(t + 0x278 + i))
            setDw(t + 0x338, dw(0x25a))
            setDw(t + 0x33a, dw(t + 0x286))
        }
        setVolume(di, 0)
        keyOff(di)
        setDw(t + 0x284, v)
        di = (di + 1) and 0xffff
    }
    setDw(c + 0x3c, dw(c + 0x3c) or 1)
    val next = ((dw(0x23c).toLong() shl 16) or dw(0x23a).toLong()) + 3
    setDw(c + 0x32, next.toInt()); setDw(c + 0x34, (next ushr 16).toInt())
    setDw(0x236, busy)
    return 1
}

/** 1878:0f88 - music from the ADLX block at [seg]:[off], faded in over [fade] steps (0: at once) to [vol]. */
fun Game.libMusic(off: Int, seg: Int, fade: Int, vol: Int): Int {
    if (dw(0x26a) and 1 == 0) return 0xffff
    fadeStop()
    if (libLoad(0xc, off, seg, 0).toShort() < 0) return 0xffff
    setDw(0x256, 1)
    if (fade == 0) {
        libStart(0xc, vol)
        libSetMusicVolume(vol)
    } else {
        libStart(0xc, 0)
        setDw(0x24a, 0); setDw(0x24c, vol)
        setDw(0x246, 0); setDw(0x248, fade)
        fadeStart()
    }
    setDw(0x25a, vol)
    setDw(0x240, dw(0x23c)); setDw(0x23e, dw(0x23a))
    return 1
}

/**
 * 1878:1059 - stops all effects and the music: at once ([fade] 0) or faded out over [fade] steps of the
 * 10 Hz timer, waiting for the fade to end (the game uses 0x11).
 */
fun Game.libControl(fade: Int): Int {
    if (dw(0x26a) and 1 == 0) return 0xffff
    for (n in 8 until 0xc) libStop(n)
    if (dw(0x256) == 0) return 0xffff
    fadeStop()
    if (fade == 0) { setDw(0x256, 0); return 1 }
    setDw(0x24c, 0); setDw(0x24a, dw(0x25a))
    setDw(0x246, 0); setDw(0x248, fade)
    fadeStart()
    while (dw(0x258) != 0) waitTimer()
    setDw(0x256, 0)
    fadeStop()
    setDw(0x274, 0)
    adlibSilence()
    return 1
}

/** 1878:1128 - music volume [vol] on all its voices. */
internal fun Game.libSetMusicVolume(vol: Int): Int {
    val saved = dw(0x274)
    if (dw(0x26a) and 1 == 0 || dw(0x256) == 0) return 0xffff
    setDw(0x25a, vol)
    setDw(0x274, 0xc)
    var v = 0
    while (dw(0x22e) > v) { setVolume(v, vol); v++ }
    setDw(0x274, saved)
    return 1
}

/** 1878:14bb - gives the voices of effect channel [n] back: other effects move up, the music gets its voices. */
private fun Game.libRelease(n: Int) {
    val c = ch(n)
    val count = dw(c + 0x36)
    val first = (9 - dw(0x26c)) and 0xffff
    val end = (first + count) and 0xffff
    setDw(c + 0x3c, 0)
    setDw(0x26c, dw(0x26c) - count)
    for (k in 8 until 0xc) {
        if (k != n && dw(ch(k) + 0x3c) and 4 != 0 && dw(ch(k) + 0x38) < dw(c + 0x38)) {
            setDw(ch(k) + 0x38, dw(ch(k) + 0x38) + count)
        }
    }
    if (dw(c + 0x38) > first) {
        var si = (dw(c + 0x38) + count - 1) and 0xffff
        while (si.toShort() >= end.toShort()) {
            val to = 0x14 * si
            val from = 0x14 * ((si - count) and 0xffff)
            for (i in 0 until 12) setDb(to + 0x278 + i, db(from + 0x278 + i))
            for (w in intArrayOf(0x284, 0x286, 0x28a, 0x288)) setDw(to + w, dw(from + w))
            setDw(0x274, dw(to + 0x288))
            val freq = dw(to + 0x28a)
            keyOff(si)
            setInstrument(si, DRIVER_SEG, (0x278 + to) and 0xffff)
            setFreq(si, freq)
            si = (si - 1) and 0xffff
        }
    }
    var si = first
    while (si.toShort() < end.toShort()) {
        val t = 0x14 * si
        if (dw(0x22e) > si) {
            setDw(t + 0x284, dw(0x25a))
            setDw(t + 0x286, dw(t + 0x33a))
            setDw(0x274, 0xc)
            setDw(t + 0x288, 0xc)
            keyOff(si)
            setInstrument(si, DRIVER_SEG, (0x32c + t) and 0xffff)
            setFreq(si, dw(t + 0x33e))
        } else {
            keyOff(si)
            setVolume(si, 0)
        }
        si = (si + 1) and 0xffff
    }
}

/** 1878:1768 - stops effect channel [n] (8..11) if it plays. */
fun Game.libStop(n: Int) {
    val busy = dw(0x236)
    if (dw(0x26a) and 1 == 0) return
    val c = ch(n)
    if (n.toShort() < 8 || n.toShort() >= 0xc || dw(c + 0x3c) and 4 == 0 || dw(c + 0x3c) and 1 == 0) return
    setDw(0x236, 1)
    setDw(c + 0x3c, 0)
    setDw(c + 0x26, dw(c + 0x1c)); setDw(c + 0x24, dw(c + 0x1a))
    var i = 0
    while (dw(c + 0x36) > i) { keyOff(dw(c + 0x38) + i); i++ }
    libRelease(n)
    setDw(0x236, busy)
}

/**
 * 1878:1852 - sound effect from the ADLX block at [seg]:[off] with volume [vol], [repeats] times (0 =
 * forever) on a free effect channel, else on the one with the lowest +0x14 word pair; returns the channel.
 */
fun Game.libEffect(off: Int, seg: Int, vol: Int, repeats: Int): Int {
    if (dw(0x26a) and 1 == 0) return 0xffff
    for (n in 8 until 0xc) {
        if (dw(ch(n) + 0x3c) and 1 == 0) {
            if (libPrepare(n, off, seg, vol, repeats).toShort() >= 0) libStart(n, vol)
            return n
        }
    }
    var best = 8
    for (n in 8 until 0xc) {
        if (below32(dw(ch(n) + 0x16), dw(ch(n) + 0x14), dw(ch(best) + 0x16), dw(ch(best) + 0x14))) best = n
    }
    libStop(best)
    if (dw(ch(best) + 0x3c) and 1 != 0) return 0xffff
    if (libPrepare(best, off, seg, vol, repeats).toShort() < 0) return 0xffff
    if (libStart(best, vol).toShort() < 0) return 0xffff
    return best
}

/** Channel [n] (8..11 an effect, 12 the music) plays (not part of the original: for exporting the sounds). */
fun Game.channelPlaying(n: Int) = dw(ch(n) + 0x3c) and 1 != 0

/**
 * Channel [n] stands at the start of its track: just loaded, or just gone back by its end event to repeat (not part
 * of the original: for exporting the sounds as loops).
 */
fun Game.channelAtStart(n: Int) = dw(ch(n) + 0x24) == dw(ch(n) + 0x1a) && dw(ch(n) + 0x26) == dw(ch(n) + 0x1c)

/** A busy wait for the next timer interrupt (1878:10ee): time passes in a live host, then the interrupt. */
internal fun Game.waitTimer() {
    host.timerWait()
    timerInterrupt()
}
