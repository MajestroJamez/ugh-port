package ugh.core.game

import ugh.core.game.Game.Companion.CODE
import ugh.core.game.Game.Companion.s16

/*
 * Everything outside level play - port of 113b:0008 .. 0c50 and the start-up / exit routines:
 * program start, intro pictures, main menu with its options, password entry, attract sequence
 * (cut scenes, credits, high scores), the game start, "bad luck" screen, ending, high score entry.
 */

/** Result of a screen that can be left with a key: Q quits, Esc (and some others) goes to the menu. */
private const val QUIT = -1

// ---------------------------------------------------------------- program

/** 113b:0008 - the whole program: start-up, intro, main menu; returns when the player quits. */
fun Game.runProgram(r: Regs = Regs()) {
    startup()
    if (!intro()) { exitProgram(); return }
    modeX192()
    mainMenu(r)
    exitProgram()
}

/** 113b:4f5e - start-up (the video mode check, CPU check and DOS/BIOS vectors are the host's business). */
fun Game.startup() {
    setD8(0x92, 3)                 // current video mode (text mode 3)
    loadHighScores()
    mem.write8(CODE, 0x4ef8, host.clockSeconds())   // 113b:4eff: seconds of the clock into the RNG state
    mem.vga.setMode(0x13)
    modeX200()
    gameKeyboardOn()
    joystickDetect()
    soundInit()
}

/** 113b:4ee6 - sound off, back to the text mode, BIOS keyboard, exit. */
fun Game.exitProgram() {
    soundControl(0x11)
    soundShutdown()
    mem.vga.setMode(d8(0x92))
    biosKeyboardOn()
    host.exit()
}

/** 113b:4f43 - sound library init (AdLib). */
fun Game.soundInit() { libInit(1) }

/** 113b:4f4e (second half) - sound library shutdown. */
fun Game.soundShutdown() { libShutdown() }

/** 113b:50c6 with no joystick connected: calibration data cleared, both joysticks absent. */
fun Game.joystickDetect() {
    setD(0x3311, 0); setD(0x3313, 0)
    setD8(0x3341, 0)
    setD(0x3343, 0xffff)
    for (o in intArrayOf(0x3321, 0x3325, 0x3323, 0x3327, 0x3329, 0x332d, 0x332b, 0x332f)) setD(o, 0)
    // 51c0: the longest calibrated axis becomes the poll limit
    var ax = u(0x3329)
    for (o in intArrayOf(0x332d, 0x332b, 0x332f)) if (ax < u(o)) ax = u(o)
    setD(0x3343, ax)
}

// ---------------------------------------------------------------- video modes

private fun Game.unchainedMode(fieldHeight: Int, pageSize: Int, offset: Int, lineCompare: Int) {
    mem.write16(CODE, 0x4600, fieldHeight)
    mem.write16(CODE, 0x4602, pageSize)
    blackPalette()
    val v = mem.vga
    v.out(0x3c4, 4); v.out(0x3c5, v.input(0x3c5) and 0xf7)
    v.out(0x3d4, 0x14); v.out(0x3d5, v.input(0x3d5) and 0xbf)
    v.out(0x3d4, 0x17); v.out(0x3d5, v.input(0x3d5) or 0x40)
    v.out(0x3d4, 0x13); v.out(0x3d5, offset)
    v.out(0x3d4, 0x18); v.out(0x3d5, lineCompare)
    v.out(0x3d4, 0x07); v.out(0x3d5, (v.input(0x3d5) and 0xef) or 0x10)
    v.out(0x3d4, 0x09); v.out(0x3d5, v.input(0x3d5) and 0xbf)
    vsync()
    v.out(0x3c0, 0x10)
    val attr = v.input(0x3c1) or 0x20
    v.out(0x3c0, attr)
    v.out(0x3c0, 0x20)
}

/** 113b:4fb3 - Mode X with 200 lines for the full-screen pictures of the intro. */
fun Game.modeX200() {
    unchainedMode(0xc8, 0x3e80, 0x28, 0x8f)
    mem.write16(CODE, 0x4604, 0)
    mem.write16(CODE, 0x4608, 0)
    mem.write16(CODE, 0x460c, 0x3e80)
    mem.write16(CODE, 0x4610, 0x7d00)
    clearVram()
    flipPages()
}

/** 113b:503b - the game's Mode X: 384 px wide pages, 192 lines play field and a split-screen status bar. */
fun Game.modeX192() {
    unchainedMode(0xc0, 0x4800, 0x30, 0x7f)
    mem.write16(CODE, 0x4604, 0)
    mem.write16(CODE, 0x4608, 0x0a88)
    mem.write16(CODE, 0x460c, 0x0a88 + 0x4f80)
    mem.write16(CODE, 0x4610, 0x0a88 + 0x4f80 + 0x4f80)
    clearVram()
    flipPages()
}

// ---------------------------------------------------------------- intro

/** 113b:4c81 - depacks a full-screen picture (ICE! at [seg]:0) and takes over its palette. */
fun Game.unpackPicture(seg: Int) {
    val len = mem.read16(seg, 4).let { (((it and 0xff) shl 8) or (it shr 8)) shl 16 } or
        mem.read16(seg, 6).let { ((it and 0xff) shl 8) or (it shr 8) }
    val src = ByteArray(len) { mem.read8(seg, it).toByte() }
    val out = ugh.core.data.Ice.unpack(src)
    val bufSeg = mem.read16(CODE, 0x4632)
    val bufOff = mem.read16(CODE, 0x4630)
    for (i in out.indices) mem.write8(bufSeg, (bufOff + i) and 0xffff, out[i].toInt())
    val palSeg = mem.read16(CODE, 0x4616)
    val palOff = mem.read16(CODE, 0x4614)
    for (i in 0 until 0x300) mem.write8(palSeg, palOff + i, mem.read8(bufSeg, 0xfa00 + i))
}

/** 113b:4634 - copies the depacked chunky picture into the draw page, 4 pixels per address over the planes. */
fun Game.showPicture() {
    var di = mem.read16(CODE, 0x460c)
    val srcSeg = u(0x9d)
    var si = u(0x9b)
    mem.vga.out(0x3c4, 2)
    repeat(mem.read16(CODE, 0x4602)) {
        for (mask in intArrayOf(1, 2, 4, 8)) {
            mem.vga.out(0x3c5, mask)
            mem.write8(0xa000, di, mem.read8(srcSeg, si)); si = (si + 1) and 0xffff
        }
        di = (di + 1) and 0xffff
    }
    mem.vga.out(0x3c5, 0x0f)
}

/** 113b:0b83 - shows a picture for [frames] or until a key: 1 = Esc, -1 = Q, 0 = timeout / other key. */
private fun Game.introPicture(seg: Int, frames: Int, r: Regs): Int {
    unpackPicture(seg)
    showPicture()
    flipPages()
    fadeIn()
    waitKeyOrTimeout(frames, r)
    fadeOut()
    return when (r.ax and 0x7f) { 1 -> 1; 0x10 -> QUIT; else -> 0 }
}

/** 113b:0bb8 - the four intro pictures; Esc skips the rest; returns false when Q quits the program. */
fun Game.intro(r: Regs = Regs()): Boolean {
    for ((seg, frames) in listOf(0x3d9a to 0x118, 0x3bf6 to 0x118, 0x3f9e to 0x8c, 0x3fb7 to 0x1a4)) {
        val res = introPicture(seg, frames, r)
        if (res < 0) return false
        if (res > 0) return true
    }
    return true
}

// ---------------------------------------------------------------- menu

/** 113b:0652 - copies the zero-terminated string number [index] of the pointer table [table] to DGROUP:[dest]. */
private fun Game.copyString(index: Int, table: Int, dest: Int) {
    var si = u(table + ((index shl 1) and 0xffff))
    var di = dest
    while (true) { val c = d8(si++); if (c == 0) break; setD8(di++, c) }
}

/** 113b:06f7 - the UGH! logo at the top of the menu screens. */
fun Game.drawLogo() {
    drawSprite(0xd7, 0x10, 0)
    drawSprite(0xd8, 0xe3, 0)
    drawSprite(0xd6, 0x61, 0)
}

/** 113b:0039 - main menu text with the current difficulty and player mode. */
private fun Game.drawMenu(r: Regs) {
    restorePlayField()
    copyString(u(0x2638), 0xda, 0xb0d)
    copyString(u(0x2634) - 1, 0xe0, 0xb19)
    r.si = 0xae4; r.cx = 0x48
    drawText(r)
    flipPages()
}

/** 113b:0063 - main menu with the attract sequence; returns when the player quits (Q). */
fun Game.mainMenu(r: Regs = Regs()) {
    while (true) {
        soundControl(0x11)
        musicStart(0x4637, 0, 0xa5)
        attract@ while (true) {
            // attract sequence: title scene, credits, second scene, high scores
            var res = titleCutScene(r)
            if (res >= 0 && res == 0) res = credits(r)
            if (res >= 0 && res == 0) res = secondCutScene(r)
            if (res >= 0 && res == 0) res = showHighScores(r)
            if (res < 0) return
            loadGamePalette()
            var redraw = true
            menu@ while (true) {
                if (redraw) {
                    setD(Game.V_WATER_ROW, 0xaf)
                    waitFrames(0x28)
                    clearVram()
                    drawLogo()
                    drawPageToBackground()
                    flipPages()
                    restorePlayField()
                    drawMenu(r)
                    fadeIn()
                    redraw = false
                }
                var cx = 0x834
                keys@ while (true) {
                    vsync()
                    cx = (cx - 1) and 0xffff
                    if (cx == 0) { fadeOut(); continue@attract }
                    readScancode(r)
                    if (r.ax shr 8 == 0) continue@keys
                    when (r.ax and 0xff) {
                        0x2e -> { fadeOut(); continue@attract }                  // C: attract sequence
                        0x10 -> { fadeOut(); return }                            // Q: quit
                        0x3f -> { fadeOut(); controlOptions(r); redraw = true; continue@menu } // F5
                        0x3e -> {                                                // F4: one player / team
                            val ax = u(0x2634) xor 3
                            setD(0x2634, ax); setD(0x2636, ax shl 1)
                            drawMenu(r); continue@menu
                        }
                        0x3d -> {                                                // F3: difficulty
                            var ax = u(0x2638) + 1
                            if (ax >= 3) ax = 0
                            setD(0x2638, ax)
                            drawMenu(r); continue@menu
                        }
                        0x3c -> { passwordEntry(r); drawMenu(r); continue@menu } // F2
                        0x3b -> {                                                // F1: play
                            fadeOut()
                            startGame(r)
                            afterGame(r)
                            break@attract
                        }
                        else -> continue@menu                                    // any other key: timeout restarts
                    }
                }
            }
        }
    }
}

/** 113b:05bb - control options text (keyboard / joystick A / joystick B for both players). */
private fun Game.drawControls(r: Regs) {
    restorePlayField()
    copyString(u(0x263f) + 1, 0xe4, 0xb73)
    copyString(u(0x2641) + 1, 0xe4, 0xb8d)
    r.si = 0xb64; r.cx = 0x50
    drawText(r)
    flipPages()
}

/** 113b:05e6 - F5: F1 / F2 cycle the control of player 1 / 2 through the connected devices. */
private fun Game.controlOptions(r: Regs) {
    setD(Game.V_WATER_ROW, 0xc8)
    waitFrames(0x28)
    drawControls(r)
    fadeIn()
    while (true) {
        var cx = 0x834
        while (true) {
            vsync()
            cx = (cx - 1) and 0xffff
            if (cx == 0) { fadeOut(); return }
            readScancode(r)
            if (r.ax shr 8 == 0) continue
            val key = r.ax and 0xff
            if (key == 1) { fadeOut(); return }
            val bx = when (key) { 0x3b -> 0; 0x3c -> 2; else -> -1 }
            if (bx < 0) break
            var ax = u(0x263f + bx)
            while (true) {
                ax = (ax + 1) and 0xffff
                if (s16(ax) >= 2) { ax = 0xffff; break }
                if (d(0x3311 + (ax shl 1)) < 0) break                    // that joystick is connected
            }
            setD(0x263f + bx, ax)
            drawControls(r)
            break
        }
    }
}

/**
 * Line editor of the password (04ca) and name (0210) entry: BIOS keyboard, blinking '?' cursor redrawn every
 * retrace, Backspace, letters (upper case), digits and optionally space. Returns the number of characters.
 */
private fun Game.lineEditor(buffer: Int, length: Int, allowSpace: Boolean, redraw: () -> Unit): Int {
    var si = 0
    var bl = 0
    while (true) {
        vsync()
        bl = (bl - 1) and 0xff
        val cursor = if (bl and 0x10 != 0) 0x3f else 0x20
        if (si < length) setD8(buffer + si, cursor)
        redraw()
        val key = biosBuffer.firstOrNull() ?: continue
        biosBuffer.removeFirst()
        var al = key and 0xff
        when {
            al == 8 -> {
                if (si == 0) continue
                if (si < length) setD8(buffer + si, 0x20)
                si--
                setD8(buffer + si, 0x20)
                bl = 0
            }
            al in 0x41..0x5a || al in 0x61..0x7a || al in 0x30..0x39 || (allowSpace && al == 0x20) -> {
                if (al in 0x61..0x7a) al -= 0x20
                if (si >= length) continue
                setD8(buffer + si, al)
                si++
                bl = 0
            }
            al == 0x0d -> return si
        }
    }
}

/** 113b:04ca - F2: password entry (20 characters, empty = NONE). */
private fun Game.passwordEntry(r: Regs) {
    biosKeyboardOn()
    restorePlayField()
    drawMenu(r)
    for (si in 0x13 downTo 0) setD8(0xb4e + si, 0x20)
    val len = lineEditor(0xb4e, 0x14, allowSpace = false) {
        restoreRect(0, 0xb0, 0x140, 0x10)
        r.si = 0xb4e; r.cx = 0xb0
        drawTextLine(r)
        flipPages()
    }
    if (len < 0x14) {
        setD8(0xb4e + len, 0x20)
        if (len == 0) { setD(0xb4e, 0x4f4e); setD(0xb50, 0x454e) } // "NONE"
    }
    gameKeyboardOn()
}

// ---------------------------------------------------------------- game start and end

/** 113b:0c07 - level from the password (0 if none matches), the opening cut scene, then the game. */
private fun Game.startGame(r: Regs) {
    var cx = 0
    var si = u(0xe8 + u(0x2636))
    outer@ while (true) {
        var bx = 0
        var mismatch = false
        while (true) {
            val al = d8(si); si = (si + 1) and 0xffff
            if (al == 0xff) { cx = 0; break@outer }
            if (al == 0) break
            bx++
            if (al != d8(0xb4d + bx)) mismatch = true
        }
        if (!mismatch) {
            // the rest of the input must be spaces
            var ok = true
            while (bx < 0x14) { if (d8(0xb4e + bx) != 0x20) { ok = false; break }; bx++ }
            if (ok) break@outer
        }
        cx++
    }
    setD(0x261c, cx)
    openingCutScene(r)
    soundControl(0x11)
    playGame(r)
}

/** 113b:0155 - after a game: the ending when all levels are done, else "bad luck"; then high scores. */
private fun Game.afterGame(r: Regs) {
    clearVram()
    if (u(0x261c) >= u(0x2624 + u(0x2636) - 2)) {
        endingCutScene(r)
    } else {
        setD(Game.V_WATER_ROW, 0xc8)
        r.si = 0xc0c; r.cx = 0x48
        drawText(r)
        flipPages()
        soundControl(0x11)
        playEffect(r, 0x426b, flags = 1, priority = 0xe1)
        fadeIn()
        waitKeyOrTimeout(0x2bc, r)
        fadeOut()
    }
    highScoreEntry(r)
}

// ---------------------------------------------------------------- attract screens

/** 113b:0720 / 0712 - scene background: level map at [mapOffset] of CODE_7, optionally with the logo. */
private fun Game.sceneBackground(water: Int, mapOffset: Int, logo: Boolean) {
    setD(Game.V_WATER_ROW, water)
    blackPalette()
    clearVram()
    loadGamePalette()
    depackToBuffer(0xa3)
    for (i in 0 until 0x140) setD8(0x264c + i, mem.read8(0x1a67, (mapOffset + i) and 0xffff))
    drawLevelTiles()
    if (logo) drawLogo()
    drawPageToBackground()
    flipPages()
    restorePlayField()
    if (d(Game.V_WATER_ROW) < 0xc0) drawWaterSurface()
    flipPages()
}

/** 113b:088d - title scene (map 0) with its cut scene; afterwards the logo gets its full width again. */
private fun Game.titleCutScene(r: Regs): Int {
    sceneBackground(0xb4, 0, logo = false)
    val res = cutScene(0x24de, r)
    r.bx = 0
    setD(0x2aed, 0)
    logoWidth(r)
    return res
}

/** 113b:08cf - second scene (map at 0x7080, with the logo). */
private fun Game.secondCutScene(r: Regs): Int {
    sceneBackground(0xc0, 0x7080, logo = true)
    return cutScene(0x1e40, r)
}

/** 113b:08c7 - opening cut scene of a game. */
private fun Game.openingCutScene(r: Regs) {
    sceneBackground(0xb4, 0, logo = false)
    cutScene(0x1792, r)
}

/** 113b:08a2 - ending after the last level. */
private fun Game.endingCutScene(r: Regs) {
    sceneBackground(0xb4, 0, logo = false)
    soundControl(0x11)
    musicStart(0x4279, 0, 0xa5)
    cutScene(0x1020, r)
}

/** 113b:07f6 - credit pages (text at DGROUP:91f), each shown until timeout or key. */
private fun Game.credits(r: Regs): Int {
    setD(Game.V_WATER_ROW, 0xc8)
    loadGamePalette()
    var si = 0x91f
    while (true) {
        waitFrames(0x28)
        clearVram()
        drawLogo()
        drawPageToBackground()
        flipPages()
        restorePlayField()
        if (d8(si) == 0xff) return 0
        // count the lines to centre the page vertically
        var bp = 0
        var p = si
        while (true) {
            val c = d8(p++)
            if (c == 0xfd || c == 0x0d) bp++
            else if (c == 0) { bp++; break }
        }
        r.cx = ((((0x90 - (bp shl 4)) and 0xffff) shr 1) + 0x38) and 0xffff
        r.si = si
        drawText(r)
        si = r.si
        flipPages()
        fadeIn()
        val key = waitKeyOrTimeout(0x15e, r)
        fadeOut()
        if (key) return when (r.ax and 0x7f) { 0x10 -> QUIT; 1 -> 1; else -> 0 }
    }
}

/** 113b:01b1 - the high score table; -1 when Q was pressed, else 1. */
private fun Game.showHighScores(r: Regs): Int {
    setD(Game.V_WATER_ROW, 0xc8)
    waitFrames(0x28)
    clearVram()
    drawLogo()
    r.si = 0xc7b; r.cx = 0x50
    drawText(r)
    flipPages()
    fadeIn()
    waitKeyOrTimeout(0x578, r)
    fadeOut()
    return if (r.ax and 0x7f == 0x10) QUIT else 1
}

// ---------------------------------------------------------------- cut scene engine

/**
 * 113b:08d5 - plays a cut scene: DGROUP:[list] = actor script pointers (-1 terminated). Actors use the
 * passenger arrays: 2aed/2b0d position, 2bed sprite, 2bcd delay (reload 2c6d), 2c2d callback, 2a0d script.
 * Returns -1 (Q), 1 (Esc) or 0 (other key, or the scene ended by itself).
 */
fun Game.cutScene(list: Int, r: Regs = Regs()): Int {
    cutSceneInit(list, r)
    setD(0x264a, 0)
    setD(Game.V_FADE, 0)
    setD(Game.V_FADE_STEP, 4)
    readScancode(r)
    while (true) {
        val cx = d(Game.V_FADE)
        if (cx < 0) return d(0x264a).coerceIn(-1, 1)
        if (cx <= 0x100) { fadeStep(cx); setD(Game.V_FADE, cx + d(Game.V_FADE_STEP)) } else vsync()
        cutSceneErase()
        if (d(Game.V_WATER_ROW) < 0xc0) drawWaterSurface()
        cutSceneStep(r)
        cutSceneDraw()
        flipPages()
        var leave = cutSceneFinished()
        if (!leave) {
            if (joystickPoll()) { setD(0x264a, 0); leave = true }
            else {
                readScancode(r)
                if (r.ax shr 8 != 0) {
                    setD(0x264a, when (r.ax and 0x7f) { 0x10 -> 0xffff; 1 -> 1; else -> 0 })
                    leave = true
                }
            }
        }
        if (leave && d(Game.V_FADE_STEP) >= 0) { setD(Game.V_FADE_STEP, 0xfffc); addD(Game.V_FADE, -4) }
    }
}

/** 113b:0aeb - actors from the script list, then 0b4f and the first script step. */
private fun Game.cutSceneInit(list: Int, r: Regs) {
    var bx = 0
    while (true) {
        val di = u(list + bx)
        if (di == 0xffff) { setD(0x2a0d + bx, di); break }
        val x = u(di); setD(0x2b2d + bx, x); setD(0x2b4d + bx, x); setD(0x2aed + bx, (x shl 5) and 0xffff)
        val y = u(di + 2); setD(0x2b6d + bx, y); setD(0x2b8d + bx, y); setD(0x2b0d + bx, (y shl 5) and 0xffff)
        setD(0x2c6d + bx, u(di + 4))
        setD(0x2c2d + bx, u(di + 6))
        setD(0x2a0d + bx, di + 8)
        setD(0x2bcd + bx, 1)
        setD(0x2acd + bx, 0)
        setD(0x2a4d + bx, 0)
        bx += 2
    }
    resetDrawnSprites()
    cutSceneStep(r)
}

/** 113b:09d7 - advances the scripts: sprite + move (dx, dy * 4), loops (negative count, 0x8000 = end of loop). */
private fun Game.cutSceneStep(r: Regs) {
    r.bx = 0
    while (true) {
        var si = u(0x2a0d + r.bx)
        if (si == 0xffff) return
        if (decZeroCs(0x2bcd + r.bx)) {
            setD(0x2bcd + r.bx, u(0x2c6d + r.bx))
            cmd@ while (true) {
                val ax = u(si); si = (si + 2) and 0xffff
                if (s16(ax) >= 0) {
                    setD(0x2bed + r.bx, ax)
                    addD(0x2aed + r.bx, (mem.read8(Game.DGROUP, si).toByte().toInt() shl 2)); si = (si + 1) and 0xffff
                    addD(0x2b0d + r.bx, (mem.read8(Game.DGROUP, si).toByte().toInt() shl 2)); si = (si + 1) and 0xffff
                    r.ax = ax
                    cutSceneCallback(u(0x2c2d + r.bx), r)
                    break@cmd
                }
                if (ax != 0x8000) {                         // loop start with a negative count
                    setD(0x2acd + r.bx, ax)
                    setD(0x2a2d + r.bx, si)
                    continue@cmd
                }
                val count = u(0x2acd + r.bx)
                if (count == 0) {                           // end of the script
                    si = (si - 2) and 0xffff
                    setD(0x2a4d + r.bx, 0xffff)
                    break@cmd
                }
                val next = (count + 1) and 0xffff
                setD(0x2acd + r.bx, next)
                if (next != 0) si = u(0x2a2d + r.bx)
            }
            setD(0x2a0d + r.bx, si)
        }
        r.bx += 2
    }
}

private fun Game.decZeroCs(off: Int): Boolean {
    val v = (u(off) - 1) and 0xffff
    setD(off, v)
    return v == 0
}

private fun Game.cutSceneCallback(addr: Int, r: Regs) {
    when (addr) {
        0x1c0e, 0x0b4e -> {}
        0x099f -> logoWidth(r)
        0x0989 -> if (u(0x2bed + r.bx) == 0x7fff) { setD(Game.V_FADE_STEP, 0xfffc); addD(Game.V_FADE, -4) }
        else -> error("cut scene callback %04x not ported".format(addr))
    }
}

/** 113b:099f - width of the logo sprite (0xd6) from the actor's x: the logo is revealed / wiped. */
private fun Game.logoWidth(r: Regs) {
    var ax = (0x140 - (s16(u(0x2aed + r.bx)) shr 5)) and 0xffff
    if (s16(ax) > 0x7f) ax = (ax and 0xff00) or 0x7f
    mem.write8(mem.read16(CODE, 0x462e), mem.read16(CODE, 0x462c), ax and 0xff)
}

/** 113b:0ab1 - erases the actors at their previous position (sprite 0x7fff = invisible). */
private fun Game.cutSceneErase() {
    var si = 0
    while (u(0x2a0d + si) != 0xffff) {
        val cur = u(0x2bed + si)
        val old = u(0x2c0d + si)
        setD(0x2c0d + si, cur)
        if (old != 0xffff && old != 0x7fff) eraseSprite(old, u(0x2b4d + si), u(0x2b8d + si))
        si += 2
    }
}

/** 113b:0a58 - draws the actors. */
private fun Game.cutSceneDraw() {
    var si = 0
    while (u(0x2a0d + si) != 0xffff) {
        val sprite = u(0x2bed + si)
        if (sprite != 0x7fff) {
            setD(0x2b4d + si, u(0x2b2d + si)); setD(0x2b8d + si, u(0x2b6d + si))
            val x = s16(u(0x2aed + si)) shr 5
            val y = s16(u(0x2b0d + si)) shr 5
            setD(0x2b2d + si, x); setD(0x2b6d + si, y)
            if (x < 0x140 && y < 0xc0) drawSprite(sprite, x and 0xffff, y and 0xffff)
        }
        si += 2
    }
}

/** 113b:09bc - true when every actor reached the end of its script. */
private fun Game.cutSceneFinished(): Boolean {
    var bx = 0
    while (true) {
        if (u(0x2a0d + bx) == 0xffff) return true
        bx += 2
        if (u(0x2a4b + bx) == 0) return false
    }
}

// ---------------------------------------------------------------- high scores

private fun Game.xorHighScores() { for (i in 0 until 0x6b) setD8(0xc86 + i, d8(0xc86 + i) xor 0xff) }

private fun Game.highScoreFileName(): String {
    val sb = StringBuilder()
    var p = 0xcf1
    while (true) { val c = d8(p++); if (c == 0) break; sb.append(c.toChar()) }
    return sb.toString()
}

/** 113b:0405 - loads UGH!.HI (0x6b bytes, XOR 0xff) over the default table, if the file exists. */
fun Game.loadHighScores() {
    val data = host.readFile(highScoreFileName()) ?: return
    setD(0xbb, 5)                                      // DOS file handle
    for (i in 0 until minOf(0x6b, data.size)) setD8(0xc86 + i, data[i].toInt())
    xorHighScores()
}

/** 113b:03da - writes UGH!.HI. */
private fun Game.saveHighScores() {
    xorHighScores()
    setD(0xbb, 5)
    host.writeFile(highScoreFileName(), ByteArray(0x6b) { d8(0xc86 + it).toByte() })
    xorHighScores()
}

/** 113b:0452 - is the score in DGROUP:c8 (6 digit characters) at least the one of entry [pos]? */
private fun Game.scoreAtLeast(pos: Int): Boolean {
    val bx = 0xc94 + pos * d8(0xc7)
    for (i in 0 until 6) {
        val a = d8(0xc8 + i); val b = d8(bx + i)
        if (a < b) return false
        if (a > b) return true
    }
    return true
}

/** 113b:0210 - enters a new high score (name entry, UGH!.HI), shows the table; the score is reset afterwards. */
private fun Game.highScoreEntry(r: Regs) {
    val score = (u(0x2620).toLong() shl 16) or u(0x261e).toLong()
    digits6Into(score, 0xc8)
    var nonZero = false
    for (i in 0 until 6) {
        var al = (d8(0xc8 + i) + 0x30) and 0xff
        if (al != 0x30) nonZero = true
        if (!nonZero && al == 0x30) al = 0x20
        setD8(0xc8 + i, al)
    }
    val pos = (0 until 5).firstOrNull { scoreAtLeast(it) }
    if (pos != null) {
        for (cl in 3 downTo pos) {
            val from = 0xc86 + cl * d8(0xc7)
            for (i in 0..0x13) setD8(from + 0x15 + i, d8(from + i))
        }
        val scoreAt = 0xc94 + pos * d8(0xc7)
        for (i in 5 downTo 0) setD8(scoreAt + i, d8(0xc8 + i))
        soundControl(0x11)
        musicStart(0x4279, 0, 0xa5)
        biosKeyboardOn()
        for (i in 0xb downTo 0) setD8(0xc6d + i, 0x20)
        setD(Game.V_WATER_ROW, 0x9f)
        waitFrames(0x28)
        clearVram()
        drawLogo()
        r.si = 0xc23; r.cx = 0x50
        drawText(r)
        drawPageToBackground()
        flipPages()
        restorePlayField()
        fadeIn()
        val redrawName = {
            restoreRect(0, 0xa0, 0x140, 0x10)
            r.si = 0xc6d; r.cx = 0xa0
            drawTextLine(r)
            flipPages()
        }
        val len = lineEditor(0xc6d, 0x0c, allowSpace = true, redraw = redrawName)
        if (len < 0x0c) {
            setD8(0xc6d + len, 0x20)
            redrawName()
            if (len == 0) { // 038e: the default name, as the original's immediate words
                setD(0xc6d, 0x4e49); setD(0xc6f, 0x4f43); setD(0xc71, 0x4e47); setD(0xc73, 0x5449); setD(0xc75, 0x5355)
            }
        }
        val nameAt = 0xc86 + pos * d8(0xc7)
        for (i in 0xb downTo 0) setD8(nameAt + i, d8(0xc6d + i))
        gameKeyboardOn()
        fadeOut()
        saveHighScores()
        showHighScores(r)
    }
    setD(0x261e, 0)
    setD(0x2620, 0)
}

/** 113b:3f0f into DGROUP:[at] (6 digits + 0xff). */
private fun Game.digits6Into(value: Long, at: Int) {
    var v = value
    var cl = -1
    do { cl++; v -= 0x186a0 } while (v >= 0)
    v += 0x186a0
    setD8(at, cl)
    setD8(at + 1, (v / u(0xc1)).toInt())
    var rem = (v % u(0xc1)).toInt()
    setD8(at + 2, rem / u(0xbf)); rem %= u(0xbf)
    setD8(at + 3, rem / d8(0xbe)); rem %= d8(0xbe)
    setD8(at + 4, rem / d8(0xbd)); setD8(at + 5, rem % d8(0xbd))
    setD8(at + 6, 0xff)
}
