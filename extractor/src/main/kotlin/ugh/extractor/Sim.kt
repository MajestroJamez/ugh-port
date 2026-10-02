package ugh.extractor

import ugh.core.data.Ice
import java.io.ByteArrayOutputStream
import java.io.File

/*
 * Data of the game logic for a reimplementation outside the DOS address space (the C++ core), format "UGHSIM01"
 * (described in re/notes/phase2-data.md). The logic addresses its tables by their original offsets (descriptors,
 * state handlers, animation tables, level records - the replays name them the same way), so the tables are kept
 * as blocks at their original seg:off; only the collision mask is computed, from the tiles of each level.
 */

/** One block of the file: [length] bytes that the original has at [seg]:[off] (or a computed one, see [Sim]). */
class SimBlock(val name: String, val seg: Int, val off: Int, val data: ByteArray) {
    init { require(name.length in 1..8 && name.all { it.code in 0x21..0x7e }) { "block name '$name'" } }
}

object Sim {
    const val MAGIC = "UGHSIM01"
    private const val ENTRY_SIZE = 20

    /** Page of the background / collision mask: 384 x 192 pixels (Mode X, 0x60 bytes per plane row). */
    const val MASK_WIDTH = 384
    const val MASK_HEIGHT = 192
    const val MASK_ROW_BYTES = MASK_WIDTH / 8

    /** Segment of the depacked level maps (113b:4ca5 depacks CODE_7 to 1a67:0000). */
    const val SEG_MAPS = 0x1a67

    fun blocks(exe: Exe): List<SimBlock> {
        val levels = Levels.extract(exe)
        val sprites = Sprites.table(exe).map { if (it.width > 0 && it.height > 0) Sprites.decode(exe, it) else null }
        return buildList {
            add(SimBlock("DGROUP", Layout.DGROUP, 0, exe.slice(Layout.DGROUP, 0, Layout.DGROUP_INIT_SIZE)))
            add(SimBlock("MAPS", SEG_MAPS, 0, Ice.unpack(exe.from(Layout.SEG_LEVEL_MAPS))))
            add(SimBlock("SPRITES", Layout.SEG_SPRITE_TABLE, 0, exe.slice(Layout.SEG_SPRITE_TABLE, 0, Layout.SPRITE_COUNT * 4)))
            for (level in levels.levels) add(SimBlock("MASK", Layout.DGROUP, level.record, mask(level, sprites)))
        }
    }

    /**
     * Collision mask of a level: bit 7 of the colour of every pixel of the background page after the level setup
     * (113b:3d66 draws the 20 x 16 tiles at col * 16, row * 12 in row order, colour 0 transparent, onto the cleared
     * page; colour | 0x40 below the water line leaves bit 7 alone). Rows of [MASK_ROW_BYTES], pixel x in bit
     * 7 - (x & 7) of byte x / 8.
     */
    fun mask(level: Level, sprites: List<IndexedImage?>): ByteArray {
        val page = IndexedImage(MASK_WIDTH, MASK_HEIGHT)
        for ((r, row) in level.tiles.withIndex()) for ((c, t) in row.withIndex()) {
            val sprite = sprites[t] ?: error("level ${level.record.hex()}: tile $t has no pixels")
            for (y in 0 until sprite.height) for (x in 0 until sprite.width) {
                val px = c * 16 + x
                val py = r * 12 + y
                val colour = sprite[x, y]
                if (colour != 0 && py < MASK_HEIGHT && px < MASK_WIDTH) page[px, py] = colour
            }
        }
        val out = ByteArray(MASK_ROW_BYTES * MASK_HEIGHT)
        for (y in 0 until MASK_HEIGHT) for (x in 0 until MASK_WIDTH) {
            if (page[x, y] and 0x80 != 0) out[y * MASK_ROW_BYTES + x / 8] = (out[y * MASK_ROW_BYTES + x / 8].toInt() or (0x80 shr (x and 7))).toByte()
        }
        return out
    }

    /**
     * Header "UGHSIM01", u32 block count, then per block {char[8] name (NUL padded), u16 seg, u16 off,
     * u32 file offset, u32 length}, then the data. Little endian.
     */
    fun write(blocks: List<SimBlock>, file: File) {
        val out = ByteArrayOutputStream()
        fun u16(v: Int) { out.write(v and 0xff); out.write((v shr 8) and 0xff) }
        fun u32(v: Int) { u16(v and 0xffff); u16(v ushr 16) }
        out.write(MAGIC.toByteArray(Charsets.US_ASCII))
        u32(blocks.size)
        var dataOffset = MAGIC.length + 4 + ENTRY_SIZE * blocks.size
        for (b in blocks) {
            out.write(b.name.padEnd(8, '\u0000').toByteArray(Charsets.US_ASCII))
            u16(b.seg); u16(b.off); u32(dataOffset); u32(b.data.size)
            dataOffset += b.data.size
        }
        for (b in blocks) out.write(b.data)
        file.parentFile?.mkdirs()
        file.writeBytes(out.toByteArray())
    }

    fun read(file: File): List<SimBlock> {
        val bytes = file.readBytes()
        fun u16(o: Int) = (bytes[o].toInt() and 0xff) or ((bytes[o + 1].toInt() and 0xff) shl 8)
        fun u32(o: Int) = u16(o) or (u16(o + 2) shl 16)
        require(String(bytes, 0, 8, Charsets.US_ASCII) == MAGIC) { "not a $MAGIC file: $file" }
        return (0 until u32(8)).map { i ->
            val e = 12 + ENTRY_SIZE * i
            val name = String(bytes, e, 8, Charsets.US_ASCII).trimEnd('\u0000')
            val start = u32(e + 12)
            SimBlock(name, u16(e + 8), u16(e + 10), bytes.copyOfRange(start, start + u32(e + 16)))
        }
    }

    fun maskBit(mask: ByteArray, x: Int, y: Int) = mask[y * MASK_ROW_BYTES + x / 8].toInt() and (0x80 shr (x and 7)) != 0
}
