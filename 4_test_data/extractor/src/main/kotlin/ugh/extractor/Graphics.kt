package ugh.extractor

import ugh.core.data.Ice
import java.awt.image.BufferedImage
import java.awt.image.IndexColorModel
import java.io.File
import javax.imageio.ImageIO

/** 256 entries of 6-bit VGA RGB (0..63), exactly as written to the DAC. */
class Palette(val vga: ByteArray) {
    init { require(vga.size == 768) }

    fun rgb8(index: Int, channel: Int): Int {
        val v = vga[index * 3 + channel].toInt() and 0x3f
        return (v shl 2) or (v shr 4)
    }

    fun colorModel(transparentIndex: Int = -1): IndexColorModel {
        val r = ByteArray(256) { rgb8(it, 0).toByte() }
        val g = ByteArray(256) { rgb8(it, 1).toByte() }
        val b = ByteArray(256) { rgb8(it, 2).toByte() }
        return if (transparentIndex >= 0) IndexColorModel(8, 256, r, g, b, transparentIndex) else IndexColorModel(8, 256, r, g, b)
    }

    companion object {
        /** Game palette: 128 colours from CODE_9 duplicated into 128..255 (113b:466a). */
        fun game(exe: Exe): Palette {
            val src = exe.slice(Layout.SEG_PALETTE, 0, 384)
            return Palette(src + src)
        }
    }
}

/** 8-bit indexed pixels, row-major. */
class IndexedImage(val width: Int, val height: Int, val pixels: ByteArray = ByteArray(width * height)) {
    init { require(pixels.size == width * height) }

    operator fun get(x: Int, y: Int) = pixels[y * width + x].toInt() and 0xff

    operator fun set(x: Int, y: Int, c: Int) { pixels[y * width + x] = c.toByte() }

    /** Draws with colour 0 transparent, like 113b:41fd above the water line. */
    fun blit(src: IndexedImage, x0: Int, y0: Int) {
        for (y in 0 until src.height) for (x in 0 until src.width) {
            val c = src[x, y]
            if (c != 0 && x0 + x in 0 until width && y0 + y in 0 until height) this[x0 + x, y0 + y] = c
        }
    }

    fun writePng(file: File, palette: Palette, transparentIndex: Int = -1) {
        file.parentFile?.mkdirs()
        val img = BufferedImage(width, height, BufferedImage.TYPE_BYTE_INDEXED, palette.colorModel(transparentIndex))
        img.raster.setDataElements(0, 0, width, height, pixels)
        check(ImageIO.write(img, "png", file)) { "no PNG writer" }
    }
}

data class SpriteEntry(val index: Int, val offset: Int, val width: Int, val height: Int, val bankSeg: Int)

object Sprites {
    fun table(exe: Exe): List<SpriteEntry> = (0 until Layout.SPRITE_COUNT).map { n ->
        val e = n * 4
        val bank = Layout.SPRITE_BANKS.last { n >= it.first }.second
        SpriteEntry(
            index = n,
            offset = exe.u16(Layout.SEG_SPRITE_TABLE, e),
            width = exe.u8(Layout.SEG_SPRITE_TABLE, e + 2),
            height = exe.u8(Layout.SEG_SPRITE_TABLE, e + 3),
            bankSeg = bank,
        )
    }

    /** Sprite pixels are stored column by column (LODSB then ADD DI,0x60 in 113b:41fd). */
    fun decode(exe: Exe, e: SpriteEntry): IndexedImage {
        val img = IndexedImage(e.width, e.height)
        var p = exe.fileOffset(e.bankSeg, e.offset)
        for (x in 0 until e.width) for (y in 0 until e.height) img[x, y] = exe.bytes[p++].toInt() and 0xff
        return img
    }
}

/** Full-screen ICE picture: 320x200 chunky pixels followed by its own palette (113b:4634, 113b:4c81). */
class Picture(val image: IndexedImage, val palette: Palette) {
    companion object {
        fun unpack(exe: Exe, seg: Int): Picture {
            val raw = Ice.unpack(exe.from(seg))
            require(raw.size == 0xfd00) { "unexpected picture size ${raw.size}" }
            return Picture(IndexedImage(320, 200, raw.copyOfRange(0, 64000)), Palette(raw.copyOfRange(64000, 64768)))
        }
    }
}
