package ugh.core.game

import ugh.core.game.Game.Companion.CODE

/**
 * What the ported game needs from the outside. The original waits for the vertical retrace in many places
 * (game loop, fades, captions, menus); the port calls [frame] at exactly those places, so the control flow
 * stays the original one and the host decides what a frame is (a 70 Hz tick of the window, or one frame of
 * the original in the lockstep tests).
 */
interface Host {
    /** Called where the original waits for the start of a vertical retrace (113b:44c6). */
    fun frame()

    /** Called in the original's busy loops without retrace wait (pause); the host may deliver input here. */
    fun idle() = frame()

    /** Contents of a file of the game directory (UGH!.HI), or null if missing. */
    fun readFile(name: String): ByteArray? = null

    /** Writes a file of the game directory. */
    fun writeFile(name: String, data: ByteArray) {}

    /** Seconds of the current time (DOS INT 21h AH=2Ch, DH), used once to vary the random numbers. */
    fun clockSeconds(): Int = 0

    /** The program ended (Q in the main menu or in the intro). */
    fun exit() { throw StopGame() }
}

/** Thrown by hosts to stop the game thread (end of a test, window closed). */
class StopGame : RuntimeException("game stopped")

// ---------------------------------------------------------------- waiting

/**
 * 113b:44c3 / 44c6 - wait for the next vertical retrace. The original polls the input status register
 * (port 3DA), which also resets the attribute controller flip-flop; the port does the same.
 */
fun Game.vsync() {
    mem.vga.resetFlipFlop()
    host.frame()
}

/** Keyboard input from the host: to the game's handler, or to the BIOS buffer while the BIOS handler is active. */
fun Game.keyEvent(scancode: Int) {
    if (!biosKeyboard) { keyboardInterrupt(scancode); return }
    val sc = scancode and 0xff
    if (sc == 0xe0) { biosExtended = true; return }
    val ext = biosExtended
    biosExtended = false
    if (sc and 0x80 == 0 && biosBuffer.size < 15) biosBuffer.addLast(ugh.core.hw.BiosKeys.word(sc, ext))
}

/** 113b:454c - the BIOS keyboard handler takes over (text entry), its buffer flushed. */
fun Game.biosKeyboardOn() { biosKeyboard = true; biosBuffer.clear() }

/** 113b:450a - the game's keyboard handler takes over: sequence matcher reset, BIOS buffer flushed. */
fun Game.gameKeyboardOn() {
    setD8(0x2647, 0)
    var si = 0
    while (d8(0x281c + si) != 0xff) { setD8(0x2821 + si, 0); si += 6 }
    setD(0x93, mem.read16(0, 0x24)); setD(0x95, mem.read16(0, 0x26))   // the old INT 9 vector is kept
    biosBuffer.clear()
    biosKeyboard = false
}

/** 113b:44bb - waits [frames] retraces (LOOP: 0 means 65536). */
fun Game.waitFrames(frames: Int) {
    var cx = frames
    do { vsync(); cx = (cx - 1) and 0xffff } while (cx != 0)
}

/** 113b:44db - waits for a key (or a joystick button). */
fun Game.waitKey(r: Regs = Regs()) {
    readScancode(r)
    while (true) {
        vsync()
        if (joystickPoll()) return
        readScancode(r)
        if (r.ax shr 8 != 0) return
    }
}

/**
 * 113b:4497 - waits up to [frames] retraces for a key; returns the scancode word (AX) and whether the wait
 * was cut short (CX != 0 in the original).
 */
fun Game.waitKeyOrTimeout(frames: Int, r: Regs = Regs()): Boolean {
    readScancode(r)
    var cx = frames
    while (true) {
        readScancode(r)
        if (r.ax shr 8 != 0) break
        if (joystickPoll()) { r.ax = 0x39; break }
        vsync()
        cx = (cx - 1) and 0xffff
        if (cx == 0) { r.ax = 0x39; break } // original: the timeout path also ends with MOV AX,0x39
    }
    r.cx = cx
    return cx != 0
}

// ---------------------------------------------------------------- palette fades

/** 113b:4e36 - one fade step: palette * level/256, retrace, then the DAC (colours 0..d8). */
fun Game.fadeStep(level: Int) {
    for (bx in 0x2ff downTo 0) {
        val v = mem.read8(CODE, 0x4681 + bx) * level
        mem.write8(CODE, 0x4981 + bx, (v shr 8) and 0xff)
    }
    vsync()
    loadDac(0x4981, first = 0, last = 0xd8)
}

/** 113b:4e19 - fade in (65 steps). */
fun Game.fadeIn() { var cx = 0; while (cx <= 0x100) { fadeStep(cx); cx += 4 } }

/** 113b:4e28 - fade out (65 steps). */
fun Game.fadeOut() { var cx = 0x100; while (cx >= 0) { fadeStep(cx); cx -= 4 } }

/** 113b:4e9b - blacks out all 256 colours, 32 per retrace. */
fun Game.blackPalette() {
    mem.vga.out(0x3c8, 0)
    repeat(8) {
        vsync()
        repeat(0x20 * 3) { mem.vga.out(0x3c9, 0) }
    }
}

/** 113b:466a - game palette (128 colours of CODE_9) into 113b:4681, twice (0..127 and 128..255). */
fun Game.loadGamePalette() {
    val srcSeg = u(0xb5); var src = u(0xb3)
    val dstSeg = u(0xad); var dst = u(0xab)
    repeat(0xc0) {
        val w = mem.read16(srcSeg, src); src = (src + 2) and 0xffff
        mem.write16(dstSeg, dst + 0x180, w)
        mem.write16(dstSeg, dst, w); dst = (dst + 2) and 0xffff
    }
}

/** 113b:4f96 - map mask 0xf and clears all 64 KB of every VGA plane. */
fun Game.clearVram() {
    mem.vga.out(0x3c4, 2); mem.vga.out(0x3c5, 0x0f)
    for (i in 0 until 0x10000 step 2) mem.write16(0xa000, i, 0)
}

// ---------------------------------------------------------------- keyboard and joystick

/**
 * 113b:4567 - the game's keyboard interrupt handler: remembers the raw scancode (CS:4509) and matches
 * scancode sequences (with 0xE0 prefixes) against the key table at DGROUP:281c, setting the key states
 * DGROUP:278c+slot for keyboard players.
 */
fun Game.keyboardInterrupt(scancode: Int) {
    val al = scancode and 0xff
    mem.write8(CODE, 0x4509, al)
    var partial = false
    val pos = d8(0x2647)
    var si = 0
    var matched = false
    while (d8(0x281c + si) != 0xff) {
        if (d8(0x2821 + si) != 0xff) {
            if (al != d8(0x281c + si + pos)) {
                setD8(0x2821 + si, 0xff)
            } else if (d8(0x281d + si + pos) != 0xff) {
                partial = true
            } else {
                val slot = d8(0x281f + si)
                if (d(0x263f + (slot and 2)) < 0) setD8(0x278c + slot, d8(0x2820 + si))
                matched = true
                break
            }
        }
        si += 6
    }
    if (matched || !partial) {
        setD8(0x2647, 0)
        si = 0
        while (d8(0x281c + si) != 0xff) { setD8(0x2821 + si, 0); si += 6 }
    } else {
        setD8(0x2647, (d8(0x2647) + 1) and 0xff)
    }
}

/**
 * 113b:51ec with no joystick connected (port 0x201 reads 0xFF): buttons 0, positions = DGROUP:3343.
 * Returns true if a button is pressed (never, without a joystick).
 */
fun Game.joystickPoll(): Boolean {
    setD(0x331d, 0)
    setD(0x331f, 0)
    val ax = u(0x3343)
    setD(0x331b, ax); setD(0x3317, ax); setD(0x3315, ax); setD(0x3319, ax)
    return false
}
