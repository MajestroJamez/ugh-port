package ugh.core.game

import ugh.core.hw.Memory

/**
 * The port of the UGH! game code (segment 113b of UGH.EXE).
 *
 * The port works on the same address space as the original ([Memory]: DGROUP, CS variables, VGA memory),
 * so that every routine can be checked byte for byte against the original running in the oracle.
 * Each function names the original routine it replaces. 16-bit semantics (wrap-around, SAR, signed compares)
 * are kept exactly; quirks of the original are kept as well and marked "original:".
 */
class Game(val mem: Memory) {

    // ------------------------------------------------------------ memory helpers

    /** Signed 16-bit word in DGROUP. */
    fun d(off: Int) = mem.readS16(DGROUP, off)
    fun setD(off: Int, v: Int) = mem.write16(DGROUP, off, v)
    fun addD(off: Int, v: Int) = setD(off, d(off) + v)
    fun d8(off: Int) = mem.read8(DGROUP, off)

    /** Word in the game code segment (variables kept in CS by the original). */
    fun cs(off: Int) = mem.read16(CODE, off)

    private fun outGc(index: Int, value: Int) { mem.vga.out(0x3ce, index); mem.vga.out(0x3cf, value) }

    // ------------------------------------------------------------ copter physics

    /**
     * 113b:1457 - collision probe: ORs 10 pixels of the collision mask (one VGA plane, sparse 6x20 grid
     * around the copter) and reports whether any of them is solid (colour bit 7).
     */
    fun probe(es: Int, si: Int): Boolean {
        var al = 0
        for (k in PROBE_OFFSETS) al = al or mem.read8(es, (si + k) and 0xffff)
        return al and 0x80 != 0
    }

    /**
     * 113b:1095 - physics of the copter of player [bx] (0 or 2): wind, thrust, gravity, water, movement
     * with pixel collision against the background page, bouncing, landing on pads and crash detection.
     */
    fun copterUpdate(bx: Int) {
        addD(V_ENERGY, -1)
        setD(P_IMPACT + bx, 0)
        setD(P_EFFORT + bx, 0)

        // CH: 1 = below the water line, 0 = exactly on it, negative = above (high byte of the difference)
        val depth = s16(d(P_Y + bx) - d(V_WATER_ROW) + 0x12)
        val ch = when {
            depth == 0 -> 0
            depth < 0 -> (depth shr 8).toByte().toInt()
            else -> 1
        }

        if (d(P_LANDED + bx) < 0) moveHorizontally(bx, ch)
        moveVertically(bx, ch)
    }

    private fun moveHorizontally(bx: Int, ch: Int) {
        // wind above the water (level flag +0c)
        if (ch < 0) {
            val wind = d8(V_WIND)
            if (wind != 0) {
                var ax = s16((((wind - 1) shl 1) - 1) shl 5)
                addD(P_VX + bx, ax)
                ax = ax shr 1
                if (ax < 0) ax = s16(-ax)
                addD(P_VY + bx, ax)
            }
        }
        var ax = d(P_VX + bx)
        if (d(KEY_LEFT + bx) != 0) { ax = s16(ax - 0x3f); addD(P_EFFORT + bx, 0x3f) }
        else if (d(KEY_RIGHT + bx) != 0) { ax = s16(ax + 0x3f); addD(P_EFFORT + bx, 0x3f) }
        ax = ax.coerceIn(-0x1800, 0x1800)
        setD(P_VX + bx, ax)

        ax = s16((ax shr 6) + d(P_XF + bx))
        if (ax < -0x200) { ax = -0x200; setD(P_VX + bx, 0) }
        else if (ax > 0x2600) { ax = 0x2600; setD(P_VX + bx, 0) }
        val di = ax

        val row = s16(d(P_Y + bx) * d(V_ROW_BYTES))
        val es = cs(CS_BACK_PAGE + 2)
        var si = (cs(CS_BACK_PAGE) + s16(row + ((d(P_X + bx) + 5) shr 2))) and 0xffff
        var cl = (d(P_X + bx) + 0x25) and 3
        outGc(4, cl)

        var bp = di
        if (di shr 5 != d(P_X + bx)) {
            bp = d(P_XF + bx)
            var hit = false
            if (di <= bp) {
                // moving left - original: the probe position is not advanced inside the loop
                cl--; if (cl < 0) { cl = cl and 3; si = (si - 1) and 0xffff }
                mem.vga.out(0x3cf, cl)
                while (true) {
                    if (probe(es, si)) { bp = s16((bp and 0xffe0) + 0x20); hit = true; break }
                    bp = s16(bp - 0x20)
                    if (bp <= di) break
                }
            } else {
                while (true) {
                    cl++; if (cl >= 4) { cl = cl and 3; si = (si + 1) and 0xffff }
                    mem.vga.out(0x3cf, cl)
                    if (probe(es, si)) { bp = s16(bp and 0xffe0); hit = true; break }
                    bp = s16(bp + 0x20)
                    if (bp >= di) break
                }
            }
            if (hit) {
                var v = s16(-d(P_VX + bx)) shr 1
                setD(P_VX + bx, v)
                v = s16(v shl 1)
                if (v < 0) v = s16(-v)
                setD(P_IMPACT + bx, v)
            } else {
                bp = di
            }
        }
        setD(P_XF + bx, bp)
        setD(P_X + bx, bp shr 5)
    }

    private fun moveVertically(bx: Int, ch: Int) {
        var ax = d(P_VY + bx)
        var checkUp = false
        when {
            ch > 0 -> { // in the water: buoyancy
                addD(V_ENERGY, -2)
                if (ax > 0) ax = s16(ax - 0xc1) else { ax = s16(ax - 0x15); setD(P_LANDED + bx, -1) }
            }
            ch < 0 && d(P_LANDED + bx) < 0 -> { // flying above the water: gravity, diving
                ax = s16(ax + 0x1b)
                addD(V_ENERGY, -2)
                if (d(KEY_DOWN + bx) != 0) { ax = s16(ax + 0x46); addD(P_EFFORT + bx, 0x5a) } else checkUp = true
            }
            else -> checkUp = true
        }
        if (checkUp && d(KEY_UP + bx) != 0) { // pedalling
            addD(V_ENERGY, -3)
            ax = s16(ax - 0x46)
            addD(P_EFFORT + bx, 0x5a)
            setD(P_LANDED + bx, -1)
        }
        ax = ax.coerceIn(-0x1800, 0x1800)
        setD(P_VY + bx, ax)

        ax = s16((ax shr 6) + d(P_YF + bx))
        if (ax < -0x260) { ax = -0x260; setD(P_VY + bx, 0) }
        else if (ax > 0x17e0) { ax = 0x17e0; setD(P_VY + bx, 0) }
        if (ch > 0) {
            val dx = s16((ax shr 5) - d(V_WATER_ROW) + 0x12)
            if (dx <= 0) { setD(P_VY + bx, 0); ax = s16((d(V_WATER_ROW) shl 5) - 0x240) }
        }
        val di = ax

        outGc(4, (d(P_X + bx) + 0x25) and 3)
        val row = s16(d(P_Y + bx) * d(V_ROW_BYTES))
        val es = cs(CS_BACK_PAGE + 2)
        var si = (cs(CS_BACK_PAGE) + s16(row + ((d(P_X + bx) + 5) shr 2))) and 0xffff

        var bp = di
        if (di shr 5 != d(P_Y + bx)) {
            bp = d(P_YF + bx)
            var hit = false
            if (di <= bp) {
                // moving up - original: the probe position is not advanced inside the loop
                si = (si - 0x60) and 0xffff
                while (true) {
                    if (probe(es, si)) { bp = s16((bp and 0xffe0) + 0x20); hit = true; break }
                    bp = s16(bp - 0x20)
                    if (bp <= di) break
                }
            } else {
                while (true) {
                    si = (si + 0x60) and 0xffff
                    if (probe(es, si)) { bp = s16(bp and 0xffe0); hit = true; break }
                    bp = s16(bp + 0x20)
                    if (bp >= di) break
                }
            }
            if (hit) bounceVertically(bx, bp) else bp = di
        }
        setD(P_YF + bx, bp)
        setD(P_Y + bx, bp shr 5)

        // crash: impact too hard for the difficulty
        if (d(P_IMPACT + bx) >= d(CRASH_LIMITS + 2 * d(V_DIFFICULTY)) && d(V_FADE_STEP) >= 0) {
            setD(V_FADE_STEP, -2)
            addD(V_FADE, -2)
        }
    }

    private fun bounceVertically(bx: Int, bp: Int) {
        var ax = s16(-d(P_VY + bx)) shr 1
        setD(P_VY + bx, ax)
        ax = s16(ax shl 1)
        if (ax < 0) ax = s16(-ax)
        if (ax > d(P_IMPACT + bx)) setD(P_IMPACT + bx, ax)
        if (d(P_VY + bx) >= 0) return            // hit a ceiling
        if (d(P_IMPACT + bx) >= d(CRASH_LIMITS + 2 * d(V_DIFFICULTY))) return

        // soft touch-down: find the pad under the copter
        val padY = s16((bp shr 5) + 0x14)
        var si = 0
        while (d(PAD_LEFT + si) >= 0) {
            if (d(PAD_Y + si) == padY) {
                val cx = s16(d(P_X + bx) + 0x10)
                if (cx >= d(PAD_LEFT + si) && cx <= d(PAD_RIGHT + si)) {
                    setD(P_LANDED + bx, si shr 1)
                    setD(P_VX + bx, 0)
                    setD(P_VY + bx, 0)
                    return
                }
            }
            si += 2
        }
    }

    companion object {
        const val CODE = 0x113b
        const val DGROUP = 0x6c09

        fun s16(v: Int) = v.toShort().toInt()

        // DGROUP variables (names provisional where the meaning is not fully known yet)
        const val V_ROW_BYTES = 0x00c3     // bytes per VGA row and plane (0x60 = 384 px virtual width)
        const val V_ENERGY = 0x2622        // decremented by effort every frame
        const val CRASH_LIMITS = 0x262e    // max impact speed per difficulty (3 words)
        const val V_DIFFICULTY = 0x2638    // 0 easy, 1 medium, 2 hard
        const val V_FADE = 0x27a8          // palette fade position (0..0x100)
        const val V_FADE_STEP = 0x27aa     // fade direction; negative = fading out
        const val V_WIND = 0x28f5          // level byte +0c
        const val V_WATER_ROW = 0x2903     // water surface, pixel row

        // per player arrays, index = player * 2
        const val KEY_UP = 0x278c
        const val KEY_DOWN = 0x2790
        const val KEY_LEFT = 0x2794
        const val KEY_RIGHT = 0x2798
        const val P_XF = 0x27d0            // x in 1/32 px
        const val P_YF = 0x27d4            // y in 1/32 px
        const val P_X = 0x27d8             // x in px
        const val P_Y = 0x27dc             // y in px
        const val P_EFFORT = 0x27f4        // effort this frame (animation speed)
        const val P_LANDED = 0x27f8        // pad index, -1 = flying
        const val P_VX = 0x2810            // velocity, 1/64 of the position unit per frame
        const val P_VY = 0x2814
        const val P_IMPACT = 0x2818        // strongest impact this frame

        // pads (level list A), index = pad * 2
        const val PAD_LEFT = 0x290d
        const val PAD_RIGHT = 0x2921
        const val PAD_Y = 0x2935

        // CS variables
        const val CS_BACK_PAGE = 0x4610    // far pointer to the background (collision) page in VGA memory

        private val PROBE_OFFSETS = intArrayOf(0, 3, 5, 0x720, 0x723, 0x725, 0x240, 0x245, 0x480, 0x485)
    }
}
