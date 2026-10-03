package ugh.extractor

/**
 * The collision masks of the levels: bit 7 of the colour of every pixel of the background page after the level
 * setup (113b:3d66 draws the 20 x 16 tiles at col * 16, row * 12 in row order, colour 0 transparent, onto the cleared
 * page; colour | 0x40 below the water line leaves bit 7 alone). The page is 384 x 192 px (Mode X, 0x60 bytes per
 * plane row); the game data (LogicData) keeps the 320 px the tiles cover.
 */
object Masks {
    const val WIDTH = 384
    const val HEIGHT = 192
    const val ROW_BYTES = WIDTH / 8

    /** The mask of every level, by the DGROUP offset of its record. */
    fun forLevels(exe: Exe): Map<Int, ByteArray> {
        val sprites = Sprites.table(exe).map { if (it.width > 0 && it.height > 0) Sprites.decode(exe, it) else null }
        return Levels.extract(exe).levels.associate { it.record to mask(it, sprites) }
    }

    /** Rows of [ROW_BYTES], pixel x in bit 7 - (x & 7) of byte x / 8. */
    fun mask(level: Level, sprites: List<IndexedImage?>): ByteArray {
        val page = IndexedImage(WIDTH, HEIGHT)
        for ((r, row) in level.tiles.withIndex()) for ((c, t) in row.withIndex()) {
            val sprite = sprites[t] ?: error("level ${level.record.hex()}: tile $t has no pixels")
            for (y in 0 until sprite.height) for (x in 0 until sprite.width) {
                val px = c * 16 + x
                val py = r * 12 + y
                val colour = sprite[x, y]
                if (colour != 0 && py < HEIGHT && px < WIDTH) page[px, py] = colour
            }
        }
        val out = ByteArray(ROW_BYTES * HEIGHT)
        for (y in 0 until HEIGHT) for (x in 0 until WIDTH) {
            if (page[x, y] and 0x80 != 0) out[y * ROW_BYTES + x / 8] = (out[y * ROW_BYTES + x / 8].toInt() or (0x80 shr (x and 7))).toByte()
        }
        return out
    }

    fun bit(mask: ByteArray, x: Int, y: Int) = mask[y * ROW_BYTES + x / 8].toInt() and (0x80 shr (x and 7)) != 0
}
