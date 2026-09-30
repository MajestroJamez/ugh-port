package ugh.core.hw

/**
 * The 1 MB real-mode address space shared by the original program (in the oracle) and the port:
 * UGH.EXE is loaded at segment [LOAD_SEG] exactly as DOS would load it, so every segment:offset matches the
 * Ghidra project and the port can read the original data (DGROUP, sprite banks, CS variables) in place.
 * The window A000:0000-FFFF is routed to the [Vga] once mode 13h is set.
 */
class Memory(val vga: Vga = Vga()) {
    val ram = ByteArray(0x100000)

    fun read8(addr: Int): Int =
        if (addr in 0xa0000..0xaffff && vga.mode == 0x13) vga.read(addr - 0xa0000) else ram[addr].toInt() and 0xff

    fun write8(addr: Int, v: Int) {
        if (addr in 0xa0000..0xaffff && vga.mode == 0x13) vga.write(addr - 0xa0000, v) else if (addr < 0xf0000) ram[addr] = v.toByte()
    }

    fun read8(seg: Int, off: Int) = read8(lin(seg, off))
    fun read16(seg: Int, off: Int) = read8(seg, off) or (read8(seg, off + 1) shl 8)
    fun readS16(seg: Int, off: Int) = read16(seg, off).toShort().toInt()
    fun write8(seg: Int, off: Int, v: Int) = write8(lin(seg, off), v and 0xff)
    fun write16(seg: Int, off: Int, v: Int) { write8(seg, off, v and 0xff); write8(seg, off + 1, (v shr 8) and 0xff) }

    /** Raw RAM access, bypassing the VGA window. */
    fun peek16(addr: Int) = (ram[addr].toInt() and 0xff) or ((ram[addr + 1].toInt() and 0xff) shl 8)
    fun poke16(addr: Int, v: Int) { ram[addr] = v.toByte(); ram[addr + 1] = (v shr 8).toByte() }

    /** Copies RAM and VGA state from [other] (used to hand the oracle's state to the port). */
    fun copyFrom(other: Memory) {
        System.arraycopy(other.ram, 0, ram, 0, ram.size)
        vga.copyFrom(other.vga)
    }

    companion object {
        const val LOAD_SEG = 0x1000
        fun lin(seg: Int, off: Int) = ((seg shl 4) + (off and 0xffff)) and 0xfffff
    }
}

/** DOS-style loader for the MZ executable. */
object MzLoader {
    data class Entry(val cs: Int, val ip: Int, val ss: Int, val sp: Int)

    fun load(exe: ByteArray, mem: Memory, loadSeg: Int = Memory.LOAD_SEG): Entry {
        fun u16(o: Int) = (exe[o].toInt() and 0xff) or ((exe[o + 1].toInt() and 0xff) shl 8)
        require(u16(0) == 0x5a4d) { "not an MZ executable" }
        val headerSize = u16(8) * 16
        val imageSize = (u16(4) - 1) * 512 + (if (u16(2) == 0) 512 else u16(2)) - headerSize
        System.arraycopy(exe, headerSize, mem.ram, loadSeg * 16, imageSize)
        for (i in 0 until u16(6)) {
            val off = u16(u16(0x18) + i * 4)
            val seg = u16(u16(0x18) + i * 4 + 2)
            val a = (loadSeg + seg) * 16 + off
            mem.poke16(a, mem.peek16(a) + loadSeg)
        }
        return Entry(loadSeg + u16(0x16), u16(0x14), loadSeg + u16(0x0e), u16(0x10))
    }
}
