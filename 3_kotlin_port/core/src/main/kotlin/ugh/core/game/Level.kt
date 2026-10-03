package ugh.core.game

import ugh.core.data.Ice
import ugh.core.game.Game.Companion.CODE
import ugh.core.game.Game.Companion.s16

/*
 * Level start - port of 113b:3d66 (level setup), 3976 (level load), 0664 (caption screen), the text
 * renderer 077c/0776 and the helpers they use.
 */

/** Segment of the depack buffer CODE_5 (113b:4ca5 loads it into ES). */
private const val DEPACK_SEG = 0x1a67

/** 113b:4ca5 - depacks the ICE! block at DS:SI (far pointer [ptr] in DGROUP) to 1a67:0000. */
fun Game.depackToBuffer(ptr: Int) {
    val seg = u(ptr + 2)
    val off = u(ptr)
    val packedLen = (mem.read16(seg, off + 6).let { ((it and 0xff) shl 8) or (it shr 8) }) or
        (mem.read16(seg, off + 4).let { (((it and 0xff) shl 8) or (it shr 8)) shl 16 })
    val src = ByteArray(packedLen) { mem.read8(seg, (off + it) and 0xffff).toByte() }
    val out = Ice.unpack(src)
    for (i in out.indices) mem.write8(DEPACK_SEG, i, out[i].toInt())
}

/** 113b:3976 - loads level DGROUP:261c of the current player mode into the game state. */
fun Game.loadLevel(r: Regs = Regs()) {
    var bx = u(0x2909 + ((u(0x2634) - 1) shl 1))
    bx = u(bx + (u(0x261c) shl 1))
    var si = 0x18
    while (s16(si) >= 0) { r.ax = u(bx + si); setD(0x28e9 + si, r.ax); si -= 2 }
    mem.write16(CODE, 0x45fe, r.ax)
    depackToBuffer(0xa3)
    val mapOff = mem.read16(CODE, 0x45fe)
    for (i in 0 until 0x140) setD8(0x264c + i, mem.read8(DEPACK_SEG, (mapOff + i) and 0xffff))

    // players
    fun startPos(fx: Int, fy: Int, pl: Int) {
        var ax = (u(fx) - 0x200) and 0xffff
        setD(Game.P_XF + pl, ax); setD(Game.P_X + pl, s16(ax) shr 5); setD(0x27e0 + pl, s16(ax) shr 5)
        ax = (times3quarter(u(fy)) - 0x140) and 0xffff
        setD(Game.P_YF + pl, ax); setD(Game.P_Y + pl, s16(ax) shr 5); setD(0x27e4 + pl, s16(ax) shr 5)
    }
    startPos(0x28f6, 0x28f8, 0)
    startPos(0x28fa, 0x28fc, 2)
    for (o in intArrayOf(0x27f8, 0x27fa, 0x27f0, 0x27f2)) setD(o, 0xffff)
    setD(0x27e8, 0xda); setD(0x27ea, 0xe0)
    setD(0x2800, 0xffff); setD(0x2802, 0xffff)
    setD8(0x27cd, 0xff)
    for (o in intArrayOf(0x27fc, 0x27fe, 0x2804, 0x2806, 0x2808, 0x280a, 0x2810, 0x2812, 0x2814, 0x2816)) setD(o, 0)
    var water = times3quarter(u(0x28fe))
    if (water > 0x17e0) water = 0x17e0
    setD(0x28fe, water)
    setD(Game.V_WATER_ROW, s16(water) shr 5); setD(0x2905, s16(water) shr 5)

    // pads (list A)
    si = u(0x28eb)
    var di = 0
    while (true) {
        val w0 = u(si); si += 2
        setD(0x290d + di, w0)
        if (s16(w0) < 0) break
        setD(0x2921 + di, u(si)); si += 2
        setD(0x2935 + di, times3quarter(u(si))); si += 2
        setD(0x2949 + di, u(si)); si += 2
        setD(0x295d + di, u(si)); si += 2
        setD(0x2971 + di, u(si)); si += 2
        setD(0x2985 + di, u(si)); si += 2
        si += 2
        setD(0x2999 + di, 0xffff)
        di += 2
    }

    // passengers (list B)
    si = u(0x28ed)
    di = 0
    while (true) {
        val type = u(si); si += 2
        setD(0x29ad + di, type)
        if (type == 0xffff) break
        setD(0x29cd + di, u(si)); si += 2
        setD(0x29ed + di, u(si)); si += 2
        for (o in intArrayOf(0x2bed, 0x2c0d, 0x2c2d, 0x2c4d)) setD(o + di, 0xffff)
        if (type == 0x78dc) {
            setD(0x2a0d + di, u(type + 0x10))
            val pad = u(0x29cd + di) shl 1
            setD(0x2aed + di, ((u(0x2971 + pad) - 8) shl 5) and 0xffff)
            setD(0x2b0d + di, ((u(0x2935 + pad) - 0xb) shl 5) and 0xffff)
        } else {
            setD(0x2a0d + di, u(type + 8))
        }
        di += 2
    }

    // objects (list C)
    si = u(0x28ef)
    di = 0
    while (true) {
        val type = u(si); si += 2
        setD(0x2cad + di, type)
        if (type == 0xffff) break
        when (type) {
            0x76e4 -> {
                val pad = u(si); si += 2
                setD(0x2cb7 + di, pad)
                setD(0x2cc1 + di, ((u(0x2971 + (pad shl 1)) - 0x10) shl 5) and 0xffff)
                setD(0x2ccb + di, ((u(0x2935 + (pad shl 1)) - 0x18) shl 5) and 0xffff)
                setD(0x2cd5 + di, u(si)); si += 2
                setD(0x2cf3 + di, 0x2a87)
            }
            0x7630 -> {
                si += 2
                setD(0x2cdf + di, times3half(u(si))); si += 2
                setD(0x2ce9 + di, u(si)); si += 2
                setD(0x2d57 + di, 1)
                setD(0x2cf3 + di, 0x2379)
            }
            0x76a8 -> {
                setD(0x2cc1 + di, (u(si) shl 5) and 0xffff); si += 2
                setD(0x2ccb + di, (times3quarter((u(si) shl 5) and 0xffff) - 0x2c0) and 0xffff); si += 2
                si += 4
                setD(0x2cf3 + di, 0x295b)
            }
            else -> {
                val pad = u(si); si += 2
                setD(0x2cb7 + di, pad)
                setD(0x2cc1 + di, ((u(0x2971 + (pad shl 1)) - 0x10) shl 5) and 0xffff)
                setD(0x2ccb + di, ((u(0x2935 + (pad shl 1)) - 0x16) shl 5) and 0xffff)
                si += 2
                val speed = u(si); si += 2
                setD(0x2ce9 + di, (-(-(s16(speed) shr 2) + speed)) and 0xffff)
                setD(0x2d57 + di, 0)
                setD(0x2cf3 + di, 0x25b1)
            }
        }
        di += 2
    }

    // rain
    for (bx2 in 0x180 downTo 0 step 2) { setD(0x2e8b + bx2, 0); setD(0x300d + bx2, 0) }
    if (d8(Game.V_WIND) != 0) {
        for (bx2 in 0x180 downTo 0 step 2) spawnRaindrop(bx2, r)
        repeat(0x241) { moveRain() }
    }
}

// ---------------------------------------------------------------- text

/**
 * 113b:077c - draws one line of text from DGROUP:[r.si] at y [r.cx] with the font (sprite 0x237 + char).
 * 0xFE at the start centres the line; 0x0D ends the line (y += 16), 0xFD ends it with y += 8, 0 ends the text.
 * Returns false at the end of the text (ZF of the original).
 */
fun Game.drawTextLine(r: Regs): Boolean {
    r.bx = 0
    var al = d8(r.si); r.si = (r.si + 1) and 0xffff
    r.di = r.si
    if (al == 0xfe) {
        r.bp = 0xffff
        while (true) {
            al = d8(r.si); r.si = (r.si + 1) and 0xffff
            r.bp = (r.bp + 1) and 0xffff
            if (al == 0 || al == 0x0d || al == 0xfd) break
        }
        r.bp = (r.bp shl 4) and 0xffff
        r.bx = ((0x140 - r.bp) and 0xffff) shr 1
        r.si = r.di
        al = d8(r.si); r.si = (r.si + 1) and 0xffff
    }
    while (true) {
        when (al) {
            0x0d -> { r.cx = (r.cx + 16) and 0xffff; r.ax = (r.ax and 0xff00) or al; return true }
            0xfd -> { r.cx = (r.cx + 8) and 0xffff; r.ax = (r.ax and 0xff00) or al; return true }
            0x00 -> { r.ax = r.ax and 0xff00; return false }
        }
        val ch = when (al) { 0x8e -> 0x5b; 0x99 -> 0x5c; 0x9a -> 0x5d; else -> al }
        r.ax = ch + 0x237
        drawSprite(r.ax, r.bx, r.cx)
        r.bx = (r.bx + 0x10) and 0xffff
        al = d8(r.si); r.si = (r.si + 1) and 0xffff
    }
}

/** 113b:0776 - draws lines until the end of the text. */
fun Game.drawText(r: Regs) { while (drawTextLine(r)) {} }

// ---------------------------------------------------------------- caption and setup

/** 113b:0664 - level caption: "LEVEL nn", the level's text and its password; waits for a key. */
fun Game.levelCaption(r: Regs = Regs()) {
    clearVram()
    val savedWater = u(Game.V_WATER_ROW)
    setD(Game.V_WATER_ROW, 0xaf)
    setD8(0xc8, ((d8(0x261c) + 1) and 0xff) / d8(0xbd))
    setD8(0xc9, ((d8(0x261c) + 1) and 0xff) % d8(0xbd))
    setD8(0xca, 0xff)
    setD8(0xbfd, (d8(0xc8) + 0x30) and 0xff)
    setD8(0xbfe, (d8(0xc9) + 0x30) and 0xff)
    r.cx = 0x20; r.si = 0xbf6
    drawTextLine(r)
    r.cx = (r.cx + 0x20) and 0xffff
    r.si = u(0x28f3)
    drawText(r)
    r.cx = 0xa0; r.si = 0xc00
    drawTextLine(r)
    var dl = (d8(0x261c) + 1) and 0xff
    r.si = u(0xea + ((u(0x2636) - 2) and 0xffff))
    while (true) {
        dl = (dl - 1) and 0xff
        if (dl == 0) break
        while (d8(r.si).also { r.si = (r.si + 1) and 0xffff } != 0) {}
    }
    drawTextLine(r)
    flipPages()
    soundControl(0x11)
    playEffect(r, 0x4629, flags = 1, priority = 0xe1)
    fadeIn()
    waitKey(r)
    setD(Game.V_WATER_ROW, savedWater)
    fadeOut()
}

/** 113b:4172 - draws the 20 x 16 tiles of the level map (DGROUP:[9f]) into the draw page. */
fun Game.drawLevelTiles() {
    val seg = u(0xa1)
    var si = u(0x9f)
    for (row in 0 until 16) for (col in 0 until 20) {
        val tile = mem.read8(seg, si); si = (si + 1) and 0xffff
        drawSprite(tile, col shl 4, 12 * row)
    }
}

/** 113b:3e79 - player icons of the status bar. */
fun Game.drawStatusIcons() {
    val (a, b) = if (u(0x2634) == 2) 0xb0 to 0xb1 else 0xae to 0xaf
    drawSprite(a, 0, 0xc1)
    drawSprite(b, 0xff, 0xc1)
}

/** 113b:0b4f - forgets the sprites drawn last (nothing to erase in the first frame). */
fun Game.resetDrawnSprites() {
    for (si in 8 downTo 0 step 2) { setD(0x2d43 + si, 0xffff); setD(0x2d4d + si, 0xffff) }
    for (si in 0x1e downTo 0 step 2) { setD(0x2bed + si, 0xffff); setD(0x2c0d + si, 0xffff) }
    for (si in 0x16 downTo 0 step 2) { setD(0x2d9b + si, 0xffff); setD(0x2db3 + si, 0xffff) }
}

/** 113b:3d66 - sets up a level attempt: state, status bar, level load, caption, background, first update. */
fun Game.levelSetup(r: Regs = Regs()) {
    run {
        val seg = u(0xb9); val off = u(0xb7)
        for (i in 0 until 0xc4) mem.write16(seg, (off + 2 * i) and 0xffff, 0)
    }
    setD(Game.V_ENERGY, 0x5a3b)
    setD(0x27c8, 0xffff)
    setD(Game.V_FADE_STEP, 2)
    // make the status bar redraw everything
    fun offset(v: Int): Int { var al = (v + 0xb) and 0xff; if (al >= 0x64) al -= 0x64; return al }
    setD8(0x27cb, offset(d8(0x263c)))
    setD8(0x27ca, offset(d8(0x263d)))
    setD8(0x27cc, 0x63)
    setD(0x27c6, 0x270f)
    var score = ((u(0x2620).toLong() shl 16) or u(0x261e).toLong()) + 0x1b207
    if (score > 0xf4240) score -= 0xf4240
    setD(0x27c2, (score and 0xffff).toInt()); setD(0x27c4, ((score shr 16) and 0xffff).toInt())
    setD(0x27ae, 0xc2)
    if (u(0x2634) == 2) {
        setD(0x27b0, 0x21); setD(0x27b2, 0x72); setD(0x27b4, 0xa9); setD(0x27b8, 0xc7); setD(0x27b6, 0xd7)
        setD(0x27ba, 0xe8); setD(0x27bc, 0x131); setD(0x27be, 0x102); setD(0x27c0, 0x112)
    } else {
        setD(0x27b0, 0x21); setD(0x27b2, 0x76); setD(0x27b4, 0xb1); setD(0x27b8, 0xd0); setD(0x27b6, 0xe0)
        setD(0x27ba, 0xf4); setD(0x27bc, 0x122)
    }
    loadLevel(r)
    levelCaption(r)
    clearVram()
    blackPalette()
    loadGamePalette()
    drawLevelTiles()
    drawPageToBackground()
    flipPages()
    restorePlayField()
    drawStatusIcons()
    objectsUpdate(r)
    passengersUpdate(r)
    resetDrawnSprites()
}
