package ugh.core.game

import ugh.core.game.Game.Companion.CODE
import ugh.core.game.Game.Companion.s16

/*
 * Drawing - port of the Mode X routines of 113b. Everything goes through the same VGA model as the original
 * (map mask, write modes, latches), so the video memory and the adapter state stay identical.
 *
 * CS variables (far pointers into A000): 4600 play field height, 4602 page size, 4604 status bar page,
 * 4608 display page, 460c draw page, 4610 background page (also the collision mask, colour bit 7),
 * 4628 sprite table, 4618..4624 sprite banks; 45fc / 45fe scratch of the sprite routine.
 */

private const val VGA_SEG = 0xa000

private fun Game.cs16(off: Int) = mem.read16(CODE, off)
private fun Game.setCs16(off: Int, v: Int) = mem.write16(CODE, off, v)
private fun Game.out(port: Int, v: Int) = mem.vga.out(port, v and 0xff)
private fun Game.inp(port: Int) = mem.vga.input(port)
private fun Game.vram(off: Int) = mem.read8(VGA_SEG, off and 0xffff)
private fun Game.setVram(off: Int, v: Int) = mem.write8(VGA_SEG, off and 0xffff, v)

/** 113b:41fd - draws sprite [sprite] at x [x], y [y]; rows below the water line get colour | 0x40. */
fun Game.drawSprite(sprite: Int, x: Int, y: Int) {
    var cy = y
    var di = cs16(0x460c)
    if (s16(cy) >= s16(cs16(0x4600))) { di = cs16(0x4604); cy = (cy - 0xc0) and 0xffff }
    setCs16(0x45fe, (u(Game.V_WATER_ROW) - cy) and 0xffff)
    di = (di + (s16(x) shr 2)) and 0xffff
    var t = (cy shl 5) and 0xffff
    di = (di + t) and 0xffff
    t = (t shl 1) and 0xffff
    di = (di + t) and 0xffff
    out(0x3c4, 2)
    var ah = 1 shl (x and 3)
    setCs16(0x45fc, (ah shl 8) or 0x02)
    val entry = sprite shl 2
    val tabSeg = cs16(0x462a)
    val tabOff = (cs16(0x4628) + entry) and 0xffff
    val dataOff = mem.read16(tabSeg, tabOff)
    val w = mem.read8(tabSeg, tabOff + 2)
    val h = mem.read8(tabSeg, tabOff + 3)
    val bankPtr = when {
        sprite < 0xd9 -> 0x4618
        sprite < 0x12a -> 0x461c
        sprite < 0x223 -> 0x4620
        else -> 0x4624
    }
    val bankSeg = cs16(bankPtr + 2)
    var si = (cs16(bankPtr) + dataOff) and 0xffff
    ah = cs16(0x45fc) shr 8
    val above = s16(cs16(0x45fe))
    // original: DEC CL / DEC BL loops, so a width or height of 0 means 256
    var cl = w
    do {
        out(0x3c5, ah)
        var col = di
        var bp = above
        var bl = h
        do {
            var c = mem.read8(bankSeg, si); si = (si + 1) and 0xffff
            if (c != 0) {
                if (bp < 0) c = c or 0x40
                setVram(col, c)
            }
            col = (col + 0x60) and 0xffff
            bp--
            bl = (bl - 1) and 0xff
        } while (bl != 0)
        ah = (ah shl 1) and 0x0f
        if (ah == 0) { ah = 1; di = (di + 1) and 0xffff }
        cl = (cl - 1) and 0xff
    } while (cl != 0)
    out(0x3c5, 0x0f)
}

/** 113b:43ea - restores a rectangle of the draw page from the background page (write mode 1 latch copy). */
fun Game.restoreRect(x: Int, y: Int, w: Int, h: Int) {
    out(0x3ce, 5)
    out(0x3cf, (inp(0x3cf) and 0xfc) + 1)
    var si = ((y shl 5) + (y shl 6) + (s16(x) shr 2)) and 0xffff
    var di = si
    si = (si + cs16(0x4610)) and 0xffff
    di = (di + cs16(0x460c)) and 0xffff
    val bytes = ((x and 3) + w + 3) shr 2 and 0xffff
    var rows = h
    while (true) {
        for (i in 0 until bytes) setVram(di + i, vram(si + i))
        si = (si + 0x60) and 0xffff
        di = (di + 0x60) and 0xffff
        rows = (rows - 1) and 0xffff
        if (rows == 0) break
    }
    out(0x3cf, inp(0x3cf) and 0xfc)
}

/** 113b:43b9 - erases sprite [sprite] drawn at [x], [y] (size from the sprite table). */
fun Game.eraseSprite(sprite: Int, x: Int, y: Int) {
    val tabSeg = cs16(0x462a)
    val tabOff = (sprite shl 2) and 0xffff
    restoreRect(x, y, mem.read8(tabSeg, tabOff + 2), mem.read8(tabSeg, tabOff + 3))
}

/** 113b:43a5 - restores the whole play field. */
fun Game.restorePlayField() = restoreRect(0, 0, 0x140, cs16(0x4600))

/** 113b:446f - copies the draw page to the background page. */
fun Game.drawPageToBackground() {
    out(0x3ce, 5)
    out(0x3cf, (inp(0x3cf) and 0xfc) + 1)
    val src = cs16(0x460c)
    val dst = cs16(0x4610)
    for (i in 0 until cs16(0x4602)) setVram(dst + i, vram(src + i))
    out(0x3cf, inp(0x3cf) and 0xfc)
}

/** 113b:4ebb - shows the draw page (CRTC start address) and swaps draw and display page. */
fun Game.flipPages() {
    val display = cs16(0x4608)
    val bx = cs16(0x460c)          // the page just drawn becomes visible
    setCs16(0x460c, display)
    setCs16(0x4608, bx)
    mem.vga.resetFlipFlop()        // 44d2 polls port 3DA (end of the retrace)
    out(0x3d4, 0x0c); out(0x3d5, bx shr 8)
    out(0x3d4, 0x0d); out(0x3d5, bx and 0xff)
}

/** 113b:4e36 (without its vsync wait) - scales the palette 113b:4681 by [level]/256 into 4981 and loads colours 0..d8. */
fun Game.fadePalette(level: Int) {
    for (bx in 0x2ff downTo 0) {
        val v = mem.read8(CODE, 0x4681 + bx) * level
        mem.write8(CODE, 0x4981 + bx, (v shr 8) and 0xff)
    }
    loadDac(0x4981, first = 0, last = 0xd8)
}

/** 113b:4e72 - writes colours [first]..[last] of the palette at CS:[off] to the DAC. */
fun Game.loadDac(off: Int, first: Int, last: Int) {
    out(0x3c8, first)
    var si = off + first * 3
    repeat(last - first + 1) {
        out(0x3c9, mem.read8(CODE, si)); out(0x3c9, mem.read8(CODE, si + 1)); out(0x3c9, mem.read8(CODE, si + 2))
        si += 3
    }
}

// ---------------------------------------------------------------- copter

/** 113b:418d - copter animation (speed from the effort) and drawing of player [bx]. */
fun Game.drawCopter(bx: Int) {
    var ax = d(Game.P_EFFORT + bx)
    if (ax > 0x5a) ax = 0x5a
    addD(0x27f0 + bx, -ax)
    ax = d(0x27f0 + bx)
    if (ax < 0) {
        setD(0x27f0 + bx, ax + 0xc8)
        ax = d(0x27e8 + bx) + 1
        if (ax >= d(0x0cfc + bx)) ax = d(0x0cfa + bx)
        setD(0x27e8 + bx, ax)
    }
    ax = u(0x27e8 + bx)
    val y = d(Game.P_Y + bx)
    if (y <= -0x14 || y >= 0xc0) return
    val x = d(Game.P_X + bx)
    if (x <= -0x20 || x >= 0x140) return
    drawSprite(ax, x and 0xffff, y and 0xffff)
}

/** 113b:436d - erases the copter of player [bx] at its previous position, then remembers the current one (4352). */
fun Game.eraseCopter(bx: Int) {
    val sprite = u(0x27ec + bx)
    val x = d(0x27e0 + bx)
    val y = d(0x27e4 + bx)
    if (x > -0x20 && x < 0x140 && y > -0x14 && y < 0xc0) eraseSprite(sprite, x and 0xffff, y and 0xffff)
    setD(0x27ec + bx, u(0x27e8 + bx))
    setD(0x27e0 + bx, u(Game.P_X + bx))
    setD(0x27e4 + bx, u(Game.P_Y + bx))
}

// ---------------------------------------------------------------- status bar

/** 113b:3eaa - one digit at the status bar cursor (27ac, 27ae). */
private fun Game.drawDigit(digit: Int) = drawSprite((digit and 0xff) + 0xcc, u(0x27ac), u(0x27ae))

/** 113b:3edd - draws the digits of the new number (DGROUP:c8) that differ from the old one (DGROUP:cf). */
private fun Game.drawChangedDigits() {
    var si = 0
    while (true) {
        val digit = d8(0xc8 + si)
        if (digit == 0xff) return
        if (digit != d8(0xcf + si)) drawDigit(digit)
        addD(0x27ac, 6)
        si++
    }
}

/** 113b:3f0f .. 3f43 - splits a number into decimal digits at DGROUP:[si], terminated by 0xff. */
private fun Game.digits6(value: Long, at: Int) {
    var si = at
    var v = value
    var cl = -1
    do { cl++; v -= 0x186a0 } while (v >= 0)
    v += 0x186a0
    setD8(si++, cl)
    val q = (v / u(0xc1)).toInt()
    setD8(si++, q)
    digits4((v % u(0xc1)).toInt(), si)
}

private fun Game.digits4(value: Int, at: Int) {
    var si = at
    setD8(si++, value / u(0xbf))
    val r = value % u(0xbf)
    setD8(si++, r / d8(0xbe))
    digits2(r % d8(0xbe), si)
}

private fun Game.digits2(value: Int, at: Int) {
    var si = at
    setD8(si++, value / d8(0xbd))
    setD8(si++, value % d8(0xbd))
    setD8(si, 0xff)
}

/** 113b:3f55 - redraws the parts of the status bar whose values changed, and the energy bar. */
fun Game.updateStatusBar() {
    setD(0x27ae, 0xc2)
    // lives
    if (d8(0x263c) != d8(0x27cb)) {
        val old = d8(0x27cb)
        setD8(0x27cb, d8(0x263c))
        setD(0x27ac, u(0x27b0))
        digits2(d8(0x263c), 0xc8); digits2(old, 0xcf); drawChangedDigits()
    }
    // score, at most 999999
    var score = (u(0x2620).toLong() shl 16) or u(0x261e).toLong()
    if (score > 999999) { score = 999999; setD(0x261e, 0x423f); setD(0x2620, 0xf) }
    val shownScore = (u(0x27c4).toLong() shl 16) or u(0x27c2).toLong()
    // one of: score, cargo of player 1, cargo of player 2, fare + multiplier (then the energy bar)
    if (score != shownScore) {
        setD(0x27c2, (score and 0xffff).toInt())
        setD(0x27c4, (score shr 16).toInt())
        setD(0x27ac, u(0x27b2))
        digits6(score, 0xc8); digits6(shownScore, 0xcf); drawChangedDigits()
        energyBar()
        return
    }
    if (d8(0x28f1) != d8(0x27cc)) {
        // passengers left
        val old = d8(0x27cc)
        setD8(0x27cc, d8(0x28f1))
        setD(0x27ac, u(0x27b4))
        digits2(d8(0x28f1), 0xc8); digits2(old, 0xcf); drawChangedDigits()
    }
    if (u(0x27fc) != u(0x2800)) {
        // cargo and target pad of player 1
        setD(0x2800, u(0x27fc))
        drawSprite((u(0x27fc) + 0xb2) and 0xffff, u(0x27b6), 0xc1)
        drawSprite((u(0x2804) + 0xb7) and 0xffff, u(0x27b8), 0xc1)
    } else if (u(0x2634) == 2 && u(0x27fe) != u(0x2802)) {
        setD(0x2802, u(0x27fe))
        drawSprite((u(0x27fe) + 0xbf) and 0xffff, u(0x27be), 0xc1)
        drawSprite((u(0x2806) + 0xc4) and 0xffff, u(0x27c0), 0xc1)
    } else {
        // fare of the carried passenger
        setD(0x27ac, u(0x27ba))
        val fare: Int? = when {
            u(0x27fc) != 0 -> u(0x2808)
            u(0x2634) == 2 && u(0x27fe) != 0 -> u(0x280a)
            d8(0x27cd) != 0 -> 0
            else -> null
        }
        if (fare != null) {
            val old = u(0x27c6)
            if (fare != old) {
                setD(0x27c6, fare)
                digits4(fare, 0xc8); digits4(old, 0xcf); drawChangedDigits()
            }
            setD8(0x27cd, 0)
        }
        // multiplier
        if (d8(0x263d) != d8(0x27ca)) {
            val old = d8(0x27ca)
            setD8(0x27ca, d8(0x263d))
            setD(0x27ac, u(0x27bc))
            digits2(d8(0x263d), 0xc8); digits2(old, 0xcf); drawChangedDigits()
        }
    }
    energyBar()
}

/** 113b:40ba - the energy bar (1 px per DGROUP:c5 units); running out of energy ends the life. */
private fun Game.energyBar() {
    val energy = d(Game.V_ENERGY)
    if (energy < 0) {
        setD(Game.V_ENERGY, 0xffff)
        if (d(Game.V_FADE_STEP) >= 0) { setD(Game.V_FADE_STEP, 0xfffe); addD(Game.V_FADE, -2) }
        return
    }
    val len = energy / u(0xc5)
    val shown = u(0x27a6)
    val statusSeg = u(0xa9)
    val statusOff = u(0xa7)
    if (len != shown) {
        if (len > s16(shown)) {
            val di = shown shr 2
            val ax = len - 1
            var bp = ax
            out(0x3c4, 2)
            out(0x3c5, d8(0x2618 + (ax and 3)))
            bp = bp shr 2
            var si = (statusOff + bp) and 0xffff
            fun column() { for (row in intArrayOf(0, 0x60, 0xc0)) mem.write8(statusSeg, (si + row) and 0xffff, 0x32) }
            column()
            out(0x3c5, 0x0f)
            bp -= di
            while (bp != 0) { si = (si - 1) and 0xffff; column(); bp-- }
        } else {
            out(0x3c4, 2)
            out(0x3c5, d8(0x2614 + (len and 3)))
            val si = (statusOff + (len shr 2)) and 0xffff
            for (row in intArrayOf(0, 0x60, 0xc0)) mem.write8(statusSeg, (si + row) and 0xffff, 0)
            out(0x3c5, 0x0f)
        }
    }
    setD(0x27a6, len)
}

// ---------------------------------------------------------------- water

/** 113b:2d1c - water level (moves every second frame), tints the new underwater row of the background page. */
fun Game.updateWater() {
    if (d8(0x27a2) != 0) {
        setD8(0x27a2, 0)
    } else {
        setD(0x2905, u(Game.V_WATER_ROW))
        var ax = u(0x28fe)
        val toggle = d8(0x27ce) xor 1
        setD8(0x27ce, toggle)
        if (toggle == 0) {
            ax = (ax + u(0x2900)) and 0xffff
            if (s16(ax) < 0) ax = 0
        }
        setD(0x28fe, ax)
        ax = s16(ax) shr 5 and 0xffff
        setD(Game.V_WATER_ROW, ax)
        if (ax == u(0x2905)) return
        out(0x3ce, 8); out(0x3cf, 0x40)
        out(0x3ce, 3); out(0x3cf, 0x10)
        val si = (cs16(0x4610) + (d8(0x2905) * d8(0xc3))) and 0xffff
        for (bx in 0x4f downTo 0) { vram(si + bx); setVram(si + bx, 0x40) }
        out(0x3ce, 8); out(0x3cf, 0xff)
        out(0x3ce, 3); out(0x3cf, 0)
        setD8(0x27a2, (d8(0x27a2) - 1) and 0xff)
    }
    restoreRect(0, u(0x2905), 0x140, 1)
}

/** 113b:2db9 - animated water surface: runs the compiled row writers of the original (113b:2e03, 31cd, 3597). */
fun Game.drawWaterSurface() {
    var si = u(0x27a4)
    val delay = (d8(0x27a3) - 1) and 0xff
    setD8(0x27a3, delay)
    if (delay and 0x80 != 0) {
        setD8(0x27a3, 6)
        si = (si - 1) and 0xffff
        if (s16(si) < 0) si = 2
        setD(0x27a4, si)
    }
    val row = (d8(Game.V_WATER_ROW) * d8(0xc3)) and 0xffff
    setD(0x2907, row)
    val di = (cs16(0x460c) + row) and 0xffff
    out(0x3c4, 2)
    var al = 0x01
    out(0x3c5, al)
    val regs = intArrayOf(al, 0x57, 0xc5, 0x55, 0x54, 0x58, 0x03, 0x56) // AL CL DL BL AH CH DH BH (DX = 3c5)
    var ip = cs16(0x2db3 + (si shl 1))
    // interpreter for the few instruction forms of the compiled writers
    while (true) {
        val op = mem.read8(CODE, ip)
        when (op) {
            0x88 -> {
                val modrm = mem.read8(CODE, ip + 1)
                val reg = (modrm shr 3) and 7
                val v = if (reg == 0) al else regs[reg]
                when (modrm and 0xc7) {
                    0x05 -> { setVram(di, v); ip += 2 }
                    0x45 -> { setVram(di + mem.read8(CODE, ip + 2).toByte(), v); ip += 3 }
                    else -> error("compiled water writer: unexpected MOV form %02x at %04x".format(modrm, ip))
                }
            }
            0xd0 -> { check(mem.read8(CODE, ip + 1) == 0xe0); al = (al shl 1) and 0xff; ip += 2 }
            0xee -> { out(0x3c5, al); ip++ }
            0xb0 -> { al = mem.read8(CODE, ip + 1); ip += 2 }
            0x1f -> ip++          // POP DS
            0xc3 -> return
            else -> error("compiled water writer: unexpected opcode %02x at %04x".format(op, ip))
        }
    }
}

// ---------------------------------------------------------------- rain (level flag +0c)

/** 113b:3c35 - (re)starts raindrop [bx] at a random position on the top or side edge. */
fun Game.spawnRaindrop(bx: Int, r: Regs = Regs()) {
    r.ax = (0x140 + u(Game.V_WATER_ROW)) and 0xffff
    r.ax = random(r.ax)
    if (r.ax < 0x140) {
        r.cx = r.ax
        r.ax = 0
    } else {
        r.ax = (r.ax - 0x140) and 0xffff
        r.cx = if (d8(Game.V_WIND) == 1) 0x13f else 0
    }
    r.ax = ((r.ax and 0xff) * d8(0xc3)) and 0xffff
    r.ax = (r.ax + (r.cx shr 2)) and 0xffff
    setD(0x2e8b + bx, r.ax)
    setD(0x318f + bx, r.cx and 3)
}

/** 113b:3c78 - moves every raindrop diagonally, restarting the ones that reached the water. */
fun Game.moveRain() {
    var bx = ((u(Game.V_WATER_ROW) + 1) shl 1) and 0xffff
    val bp = ((((d8(Game.V_WIND) - 1) shl 1) - 1)) and 0xffff
    while (true) {
        var ax = u(0x2e8b + bx)
        setD(0x300d + bx, ax)
        ax = (ax + 0xc0) and 0xffff
        if (bx and 2 == 0) ax = (ax + 0x60) and 0xffff
        var cx = (u(0x318f + bx) + bp + bp) and 0xffff
        if (bx and 2 == 0) cx = (cx + bp) and 0xffff
        if (cx and 0xfc != 0) ax = (ax + bp) and 0xffff
        cx = cx and 3
        if (ax >= u(0x2907)) spawnRaindrop(bx)
        else { setD(0x2e8b + bx, ax); setD(0x318f + bx, cx) }
        bx = (bx - 2) and 0xffff
        if (s16(bx) < 0) break
    }
}

/** 113b:3cde - erases the raindrops at their previous position (latch copy from the background page). */
fun Game.eraseRain() {
    out(0x3ce, 8); out(0x3cf, 0)
    val back = cs16(0x4610)
    val draw = cs16(0x460c)
    var bx = ((u(Game.V_WATER_ROW) + 1) shl 1) and 0xffff
    while (true) {
        val a = u(0x300d + bx)
        val v = vram(back + a)
        setVram(draw + a, v)
        bx = (bx - 2) and 0xffff
        if (s16(bx) < 0) break
    }
    out(0x3cf, 0xff)
}

/** 113b:3d19 - draws the raindrops (colour 0x5b). */
fun Game.drawRain() {
    var bx = (u(Game.V_WATER_ROW) shl 1) and 0xffff
    out(0x3c4, 2)
    val draw = cs16(0x460c)
    while (true) {
        out(0x3c5, (1 shl (u(0x318f + bx) and 0x1f)) and 0xff)
        setVram(draw + u(0x2e8b + bx), 0x5b)
        bx = (bx - 2) and 0xffff
        if (s16(bx) < 0) break
    }
    out(0x3c5, 0x0f)
}

/**
 * 113b:4f09 - the game's random number generator: returns a number 0 .. [range]-1.
 * State: 4 words at CS:4ef7..4efe, updated by an add-with-carry chain.
 */
fun Game.random(range: Int): Int {
    var ax = (range + cs16(0x4ef7))
    var carry = ax shr 16
    ax = ax and 0xffff
    var dx = cs16(0x4ef9) + ax + carry
    carry = dx shr 16; dx = dx and 0xffff
    setCs16(0x4ef9, dx)
    var cx = cs16(0x4efb) + dx + carry
    carry = cx shr 16; cx = cx and 0xffff
    setCs16(0x4efb, cx)
    var dx2 = cs16(0x4efd) + cx + carry
    carry = dx2 shr 16; dx2 = dx2 and 0xffff
    setCs16(0x4efd, dx2)
    ax = (ax + dx2 + carry) and 0xffff
    setCs16(0x4ef7, ax)
    return ((ax.toLong() * range) shr 16).toInt() and 0xffff
}
