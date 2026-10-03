package ugh.extractor

/**
 * The load image of UGH.EXE, addressed the same way as in the Ghidra project:
 * load base = segment 0x1000, so `seg:off` here equals the address shown in Ghidra.
 */
class Exe(val bytes: ByteArray) {

    init {
        require(bytes.size > 0x40 && bytes[0] == 'M'.code.toByte() && bytes[1] == 'Z'.code.toByte()) { "not an MZ executable" }
        require(u16raw(8) * 16 == HEADER_SIZE) { "unexpected MZ header size ${u16raw(8) * 16}" }
    }

    private fun u16raw(o: Int) = (bytes[o].toInt() and 0xff) or ((bytes[o + 1].toInt() and 0xff) shl 8)

    fun fileOffset(seg: Int, off: Int = 0): Int = HEADER_SIZE + (seg - LOAD_SEG) * 16 + off

    fun u8(seg: Int, off: Int): Int = bytes[fileOffset(seg, off)].toInt() and 0xff

    fun u16(seg: Int, off: Int): Int = u8(seg, off) or (u8(seg, off + 1) shl 8)

    /** Signed 16-bit word, the way the x86 code sees it. */
    fun s16(seg: Int, off: Int): Int = u16(seg, off).toShort().toInt()

    fun slice(seg: Int, off: Int, length: Int): ByteArray {
        val start = fileOffset(seg, off)
        return bytes.copyOfRange(start, start + length)
    }

    fun from(seg: Int, off: Int = 0): ByteArray = bytes.copyOfRange(fileOffset(seg, off), bytes.size)

    companion object {
        const val HEADER_SIZE = 0xA00
        const val LOAD_SEG = 0x1000
    }
}

/** Segments and DGROUP offsets identified in phase 1 (see re/notes/phase1-map.md). */
object Layout {
    const val SEG_LEVEL_MAPS = 0x2a38      // CODE_7, ICE packed, 100 maps x 320 B
    const val SEG_PALETTE = 0x3bc6         // CODE_9, 768 B (128 colours used)
    const val SEG_SPRITE_TABLE = 0x6b63    // CODE_27, 664 x {u16 off, u8 w, u8 h}
    const val SPRITE_COUNT = 664
    const val DGROUP = 0x6c09              // CODE_28
    const val DGROUP_INIT_SIZE = 0x7e2e    // initialized part of DGROUP

    /** Sprite pixel banks: first sprite number -> segment (113b:4268..428f). */
    val SPRITE_BANKS = listOf(0x000 to 0x2e1f, 0x0d9 to 0x47a7, 0x12a to 0x538a, 0x223 to 0x5e83)

    /** Intro pictures in the order shown by 113b:0bb8. */
    val INTRO_IMAGES = listOf("intro1_playbyte" to 0x3d9a, "intro2_bonespark" to 0x3bf6, "intro3_present" to 0x3f9e, "title_ugh" to 0x3fb7)

    /** ADLX (AdLib) blocks CODE_14..CODE_23. */
    val ADLX_BLOCKS = listOf(0x425b, 0x4260, 0x4265, 0x426b, 0x4274, 0x4279, 0x4444, 0x4629, 0x4632, 0x4637)

    const val LEVELS_ONE_PLAYER = 0x3349   // DGROUP:[2909]
    const val LEVELS_TEAM = 0x33d5         // DGROUP:[290b]
    const val PASSWORDS_ONE_PLAYER = 0x00ee // DGROUP:[0xea]
    const val PASSWORDS_TEAM = 0x0492       // DGROUP:[0xec]
    const val LEVEL_RECORD_SIZE = 26
    const val MAP_WIDTH = 20
    const val MAP_HEIGHT = 16
}
