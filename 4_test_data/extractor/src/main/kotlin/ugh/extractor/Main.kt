package ugh.extractor

import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import java.security.MessageDigest
import kotlin.system.exitProcess

/** SHA-256 of the UGH.EXE this extractor was written against; all offsets in [Layout] are specific to it. */
const val KNOWN_EXE_SHA256 = "ef93d2cd5eb558f6a7d0007e0109f2e9ee952dc125389d7a6256646087636d7c"

@Serializable
data class SpriteInfo(val index: Int, val width: Int, val height: Int, val bankSeg: Int, val offset: Int, val file: String?)

@Serializable
data class Manifest(
    val sourceSha256: String,
    val sprites: Int,
    val levels: Int,
    val levelsOnePlayer: Int,
    val levelsTeam: Int,
    val introImages: List<String>,
    val adlx: List<String>,
)

private val json = Json { prettyPrint = true; encodeDefaults = true }

fun main(args: Array<String>) {
    if (args.size != 2) {
        System.err.println("usage: extractor <UGH.EXE> <output dir>")
        exitProcess(2)
    }
    val exe = readKnownExe(File(args[0]))
    val out = File(args[1])
    val manifest = extractAll(exe, out)
    println("Extracted ${manifest.sprites} sprites, ${manifest.levels} levels " +
        "(${manifest.levelsOnePlayer} one player, ${manifest.levelsTeam} team), " +
        "${manifest.introImages.size} pictures, ${manifest.adlx.size} ADLX blocks into ${out.absolutePath}")
}

/** The UGH.EXE in [file]; exits when it is another version than [KNOWN_EXE_SHA256]. */
fun readKnownExe(file: File): Exe {
    val exe = Exe(file.readBytes())
    val hash = sha256(exe.bytes)
    if (hash != KNOWN_EXE_SHA256) {
        System.err.println("${file.absolutePath} is a different UGH.EXE version (sha256 $hash), offsets would not match")
        exitProcess(1)
    }
    return exe
}

fun extractAll(exe: Exe, out: File): Manifest {
    out.mkdirs()
    val palette = Palette.game(exe)
    File(out, "palette.vga").writeBytes(palette.vga)

    // sprites: indexed PNG (index 0 transparent), original palette indices preserved
    val table = Sprites.table(exe)
    val images = table.map { Sprites.decode(exe, it) }
    val infos = table.mapIndexed { i, e ->
        // some table slots are empty (width or height 0)
        val name = if (e.width > 0 && e.height > 0) "sprites/%03d.png".format(i) else null
        if (name != null) images[i].writePng(File(out, name), palette, transparentIndex = 0)
        SpriteInfo(e.index, e.width, e.height, e.bankSeg, e.offset, name)
    }
    File(out, "sprites.json").writeText(json.encodeToString(infos))
    spriteSheet(images).writePng(File(out, "preview/sprites.png"), palette, transparentIndex = 0)

    // levels
    val levelSet = Levels.extract(exe)
    File(out, "levels.json").writeText(json.encodeToString(levelSet))
    for ((i, rec) in levelSet.onePlayer.withIndex()) renderMap(levelSet.levels.first { it.record == rec }, images)
        .writePng(File(out, "preview/levels-1p/%02d.png".format(i + 1)), palette)
    for ((i, rec) in levelSet.team.withIndex()) renderMap(levelSet.levels.first { it.record == rec }, images)
        .writePng(File(out, "preview/levels-team/%02d.png".format(i + 1)), palette)

    // full-screen pictures
    val intro = Layout.INTRO_IMAGES.map { (name, seg) ->
        val pic = Picture.unpack(exe, seg)
        pic.image.writePng(File(out, "pictures/$name.png"), pic.palette)
        "pictures/$name.png"
    }

    // AdLib music / effects, raw until the ADLX format is decoded
    val adlx = Layout.ADLX_BLOCKS.map { seg ->
        val name = "adlx/%04x.adlx".format(seg)
        val nextSeg = (Layout.ADLX_BLOCKS + 0x47a7).first { it > seg }
        File(out, name).apply { parentFile.mkdirs() }.writeBytes(exe.slice(seg, 0, (nextSeg - seg) * 16))
        name
    }

    // DGROUP initialized image, referenced by original offsets during the faithful port
    File(out, "dgroup.bin").writeBytes(exe.slice(Layout.DGROUP, 0, Layout.DGROUP_INIT_SIZE))

    // everything the game logic needs, for the C++ logic (logic/)
    LogicData.write(exe, File(out, "logic/ugh-data.ugd"))

    val manifest = Manifest(sha256(exe.bytes), infos.size, levelSet.levels.size, levelSet.onePlayer.size, levelSet.team.size, intro, adlx)
    File(out, "manifest.json").writeText(json.encodeToString(manifest))
    return manifest
}

fun renderMap(level: Level, sprites: List<IndexedImage>): IndexedImage {
    val img = IndexedImage(320, 192)
    for ((r, row) in level.tiles.withIndex()) for ((c, t) in row.withIndex()) img.blit(sprites[t], c * 16, r * 12)
    return img
}

/** Shelf-packed preview of all sprites in index order (index 0 = transparent background). */
private fun spriteSheet(images: List<IndexedImage>, width: Int = 640, gap: Int = 3): IndexedImage {
    val pos = ArrayList<Pair<Int, Int>>()
    var x = 0
    var y = 0
    var rowH = 0
    for (img in images) {
        if (x > 0 && x + img.width > width) { x = 0; y += rowH + gap; rowH = 0 }
        pos += x to y
        x += img.width + gap
        rowH = maxOf(rowH, img.height)
    }
    val sheet = IndexedImage(width, y + rowH)
    images.forEachIndexed { i, img -> sheet.blit(img, pos[i].first, pos[i].second) }
    return sheet
}

fun sha256(s: String): String = sha256(s.toByteArray(Charsets.UTF_8))

fun sha256(b: ByteArray): String = MessageDigest.getInstance("SHA-256").digest(b).joinToString("") { "%02x".format(it) }
