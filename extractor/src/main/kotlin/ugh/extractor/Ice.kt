package ugh.extractor

/**
 * Faithful port of the Pack-Ice depacker at 113b:4cad..4ddc in UGH.EXE.
 * Decodes backwards from the end of the packed data, exactly like the x86 code,
 * including its 16-bit register arithmetic.
 */
object Ice {
    // tables at 113b:4ddd .. 113b:4e18
    private val DIRECT_BITS = intArrayOf(0x000e, 0x0007, 0x0002, 0x0001, 0x0001)
    private val DIRECT_BASE = intArrayOf(0x010d, 0x000e, 0x0007, 0x0004, 0x0001)
    private val DIRECT_MARK = intArrayOf(0x7fff, 0x00ff, 0x0007, 0x0003, 0x0003)
    private val LEN_BITS = intArrayOf(0x09, 0x01, 0x00, -1, -1)       // index = BP + 1, BP = -1..3
    private val LEN_BASE = intArrayOf(0x08, 0x04, 0x02, 0x01, 0x00)
    private val OFF_BITS = intArrayOf(0x0b, 0x04, 0x07)               // index = BP + 1, BP = -1..1
    private val OFF_BASE = intArrayOf(0x011f, -1, 0x001f)

    const val MAGIC = 0x49434521 // "ICE!"

    fun isIce(data: ByteArray) = data.size >= 12 && be32(data, 0) == MAGIC

    fun packedLength(data: ByteArray) = be32(data, 4)

    fun unpack(src: ByteArray): ByteArray {
        require(isIce(src)) { "not ICE! data" }
        val out = ByteArray(be32(src, 8))
        Reader(src, be32(src, 4) - 1).run {
            var di = out.size - 1
            while (true) {
                if (bit() == 1) {
                    var dx = 0
                    if (bit() == 1) {
                        var i = 4
                        while (true) {
                            dx = bits(DIRECT_BITS[i])
                            if (dx != DIRECT_MARK[i] || i == 0) break
                            i--
                        }
                        dx = (dx + DIRECT_BASE[i]) and 0xffff
                    }
                    do {
                        out[di--] = src[si--]
                        dx = (dx - 1).toShort().toInt()
                    } while (dx >= 0)
                }
                if (di == -1) break

                var bp = 3
                while (bit() == 1) if (--bp < 0) break
                var dx = if (LEN_BITS[bp + 1] >= 0) bits(LEN_BITS[bp + 1]) else 0
                var bx = (LEN_BASE[bp + 1] + dx) and 0xffff
                if (bx != 0) {
                    bp = 1
                    while (bit() == 1) if (--bp < 0) break
                    dx = (bits(OFF_BITS[bp + 1]) + OFF_BASE[bp + 1]) and 0xffff
                    if (dx.toShort() < 0) dx = (dx - bx) and 0xffff
                } else {
                    var cx = 5
                    var base = -1
                    if (bit() == 1) { cx = 8; base = 0x3f }
                    dx = (bits(cx) + base) and 0xffff
                }
                var s = (di + 2 + bx + dx) and 0xffff
                out[di--] = out[s--]
                do {
                    out[di--] = out[s--]
                    bx = (bx - 1).toShort().toInt()
                } while (bx >= 0)
            }
        }
        return out
    }

    /** Bit reader with the 68k-style sentinel bit buffer (AH in the x86 code). */
    private class Reader(val src: ByteArray, var si: Int) {
        private var ah = src[si--].toInt() and 0xff

        fun bit(): Int {
            var cf = (ah shr 7) and 1
            ah = (ah shl 1) and 0xff
            if (ah == 0) {
                ah = src[si--].toInt() and 0xff
                val ncf = (ah shr 7) and 1
                ah = ((ah shl 1) or cf) and 0xff
                cf = ncf
            }
            return cf
        }

        /** Reads cx+1 bits (DEC CX / JNS loop in 113b:4d37). */
        fun bits(count: Int): Int {
            var cx = count
            var dx = 0
            do {
                dx = ((dx shl 1) or bit()) and 0xffff
                cx--
            } while (cx >= 0)
            return dx
        }
    }

    private fun be32(b: ByteArray, o: Int) =
        ((b[o].toInt() and 0xff) shl 24) or ((b[o + 1].toInt() and 0xff) shl 16) or
            ((b[o + 2].toInt() and 0xff) shl 8) or (b[o + 3].toInt() and 0xff)
}
