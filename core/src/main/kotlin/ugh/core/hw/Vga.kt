package ugh.core.hw

/**
 * VGA as used by UGH.EXE: mode 13h switched to unchained Mode X (113b:503b).
 * Emulates planar memory with latches, write modes 0-3 with rotate/ALU/bit mask, read modes 0-1,
 * sequencer map mask, CRTC start address / offset / line compare and the DAC.
 */
class Vga {
    val planes = Array(4) { ByteArray(0x10000) }
    private val latch = IntArray(4)

    val seq = IntArray(8)
    val gc = IntArray(9)
    val crtc = IntArray(0x19)
    val attr = IntArray(0x15)
    private var seqIndex = 0
    private var gcIndex = 0
    private var crtcIndex = 0
    private var attrIndex = 0
    private var attrFlipFlop = false

    /** DAC: 256 x RGB, 6 bit values. */
    val dac = IntArray(768)
    private var dacWrite = 0
    private var dacRead = 0

    var mode = 3
        private set

    fun setMode(m: Int) {
        mode = m
        if (m == 0x13) {
            seq[2] = 0x0f; seq[4] = 0x0e          // chain 4
            gc.fill(0); gc[5] = 0x40; gc[6] = 0x05; gc[7] = 0x0f; gc[8] = 0xff
            crtc.fill(0); crtc[0x09] = 0x41; crtc[0x13] = 0x28; crtc[0x14] = 0x40; crtc[0x17] = 0xa3
            crtc[0x18] = 0xff; crtc[0x07] = 0x1f
            for (p in planes) p.fill(0)
        }
    }

    private val chain4 get() = seq[4] and 0x08 != 0

    /** Full copy of the adapter state (memory, latches, registers, DAC). */
    fun copyFrom(o: Vga) {
        for (p in 0..3) System.arraycopy(o.planes[p], 0, planes[p], 0, 0x10000)
        o.latch.copyInto(latch); o.seq.copyInto(seq); o.gc.copyInto(gc); o.crtc.copyInto(crtc); o.attr.copyInto(attr)
        o.dac.copyInto(dac)
        seqIndex = o.seqIndex; gcIndex = o.gcIndex; crtcIndex = o.crtcIndex; attrIndex = o.attrIndex
        attrFlipFlop = o.attrFlipFlop; dacWrite = o.dacWrite; dacRead = o.dacRead; mode = o.mode
    }

    /** Describes the first difference to [o] (memory, latches, registers), or null if identical. */
    fun diff(o: Vga): String? {
        for (p in 0..3) if (!planes[p].contentEquals(o.planes[p])) {
            val i = (0 until 0x10000).first { planes[p][it] != o.planes[p][it] }
            return "VGA plane $p offset %04x: %02x vs %02x".format(i, planes[p][i], o.planes[p][i])
        }
        if (!latch.contentEquals(o.latch)) return "VGA latches ${latch.toList()} vs ${o.latch.toList()}"
        if (!seq.contentEquals(o.seq) || seqIndex != o.seqIndex) return "VGA sequencer differs"
        if (!gc.contentEquals(o.gc) || gcIndex != o.gcIndex) return "VGA graphics controller ${gc.toList()}/$gcIndex vs ${o.gc.toList()}/${o.gcIndex}"
        if (!crtc.contentEquals(o.crtc)) return "VGA CRTC differs"
        if (!dac.contentEquals(o.dac)) return "VGA DAC differs"
        return null
    }

    // ------------------------------------------------------------ memory

    fun read(offset: Int): Int {
        if (chain4) return planes[offset and 3][offset and 0xfffc].toInt() and 0xff
        for (p in 0..3) latch[p] = planes[p][offset].toInt() and 0xff
        return if (gc[5] and 0x08 == 0) {
            latch[gc[4] and 3]
        } else {
            // read mode 1: colour compare
            var r = 0
            for (bit in 0..7) {
                var match = true
                for (p in 0..3) if (gc[7] and (1 shl p) != 0) {
                    val pb = (latch[p] shr bit) and 1
                    if (pb != (gc[2] shr p) and 1) match = false
                }
                if (match) r = r or (1 shl bit)
            }
            r
        }
    }

    fun write(offset: Int, value: Int) {
        if (chain4) {
            if (seq[2] and (1 shl (offset and 3)) != 0) planes[offset and 3][offset and 0xfffc] = value.toByte()
            return
        }
        val writeMode = gc[5] and 3
        val rotate = gc[3] and 7
        val func = (gc[3] shr 3) and 3
        val v = ((value shr rotate) or (value shl (8 - rotate))) and 0xff
        for (p in 0..3) {
            if (seq[2] and (1 shl p) == 0) continue
            val out = when (writeMode) {
                1 -> latch[p]
                else -> {
                    var d: Int
                    var mask = gc[8]
                    when (writeMode) {
                        0 -> d = if (gc[1] and (1 shl p) != 0) (if (gc[0] and (1 shl p) != 0) 0xff else 0) else v
                        2 -> d = if (value and (1 shl p) != 0) 0xff else 0
                        else -> { mask = mask and v; d = if (gc[0] and (1 shl p) != 0) 0xff else 0 }
                    }
                    if (writeMode != 3) d = when (func) {
                        1 -> d and latch[p]
                        2 -> d or latch[p]
                        3 -> d xor latch[p]
                        else -> d
                    }
                    (d and mask) or (latch[p] and mask.inv() and 0xff)
                }
            }
            planes[p][offset] = out.toByte()
        }
    }

    // ------------------------------------------------------------ ports

    fun out(port: Int, v: Int) {
        when (port) {
            0x3c0 -> {
                if (!attrFlipFlop) attrIndex = v and 0x1f else if (attrIndex < attr.size) attr[attrIndex] = v
                attrFlipFlop = !attrFlipFlop
            }
            0x3c4 -> seqIndex = v and 7
            0x3c5 -> seq[seqIndex] = v
            0x3c7 -> dacRead = (v and 0xff) * 3
            0x3c8 -> dacWrite = (v and 0xff) * 3
            0x3c9 -> { dac[dacWrite] = v and 0x3f; dacWrite = (dacWrite + 1) % 768 }
            0x3ce -> gcIndex = v and 0x0f
            0x3cf -> if (gcIndex < gc.size) gc[gcIndex] = v
            0x3d4 -> crtcIndex = v and 0x1f
            0x3d5 -> if (crtcIndex < crtc.size) crtc[crtcIndex] = v
        }
    }

    fun input(port: Int): Int = when (port) {
        0x3c1 -> if (attrIndex < attr.size) attr[attrIndex] else 0
        0x3c5 -> seq[seqIndex]
        0x3c9 -> { val r = dac[dacRead]; dacRead = (dacRead + 1) % 768; r }
        0x3cf -> if (gcIndex < gc.size) gc[gcIndex] else 0
        0x3d5 -> if (crtcIndex < crtc.size) crtc[crtcIndex] else 0
        else -> 0xff
    }

    /** Reading input status 1 resets the attribute flip-flop. */
    fun resetFlipFlop() { attrFlipFlop = false }

    // ------------------------------------------------------------ display

    val startAddress get() = (crtc[0x0c] shl 8) or crtc[0x0d]
    val lineOffsetBytes get() = crtc[0x13] * 2
    val lineCompare get() = crtc[0x18] or ((crtc[0x07] and 0x10) shl 4) or ((crtc[0x09] and 0x40) shl 3)

    /** Renders the visible 320x200 frame as palette indices (Mode X, double scanned, with split screen). */
    fun renderIndexed(): ByteArray {
        val out = ByteArray(320 * 200)
        if (chain4) {
            for (i in 0 until 64000) out[i] = planes[i and 3][i and 0xfffc]
            return out
        }
        val stride = lineOffsetBytes
        val splitRow = (lineCompare + 1) / 2
        for (y in 0 until 200) {
            val base = if (y >= splitRow) (y - splitRow) * stride else startAddress + y * stride
            for (x in 0 until 320) out[y * 320 + x] = planes[x and 3][(base + (x shr 2)) and 0xffff]
        }
        return out
    }
}
