package ugh.extractor

import kotlinx.serialization.Serializable

/*
 * Level data as stored in DGROUP. All word values are kept RAW (signed 16-bit, Amiga coordinate space);
 * the conversions done at load time by 113b:3976 (Y * 3/4 via 113b:3d4d, << 5 fixed point, pad-relative
 * offsets...) belong to the game core, which must reproduce them with the same integer arithmetic.
 * Field names are provisional until the corresponding game logic is reverse engineered.
 */

@Serializable
data class LevelSet(
    /** Level record ids (DGROUP offsets) in play order, 1 player. */
    val onePlayer: List<Int>,
    /** Level record ids in play order, TEAM MODE (2 players). */
    val team: List<Int>,
    val passwordsOnePlayer: List<String>,
    val passwordsTeam: List<String>,
    val levels: List<Level>,
)

@Serializable
data class Level(
    /** DGROUP offset of the 26-byte record, used as the level id. */
    val record: Int,
    /** +00 offset of the map inside the unpacked CODE_7. */
    val mapOffset: Int,
    /** 16 rows x 20 tile numbers (= sprite numbers). */
    val tiles: List<List<Int>>,
    /** +02 list A: landing pads (-> DGROUP:290d.., max 10). */
    val pads: List<Pad>,
    /** +04 list B: passengers (-> DGROUP:29ad.., max 16). */
    val passengers: List<Passenger>,
    /** +06 list C: active objects / enemies (-> DGROUP:2cad.., max 5). */
    val objects: List<LevelObject>,
    /** +08 word, compared with DGROUP:27cc in 113b:3fcb. */
    val field08: Int,
    /** +0a pointer to the level caption shown before the level (113b:0698). */
    val captionPtr: Int,
    /** Caption bytes up to the terminating 0 (0xFE = format code, 0x0D = new line). */
    val captionRaw: List<Int>,
    val caption: String,
    /** +0c byte flag (DGROUP:28f5), enables 113b:3cde each frame. */
    val flag0c: Int,
    /** +0d..+14 start positions of player 1 and 2 (raw). */
    val start1X: Int,
    val start1Y: Int,
    val start2X: Int,
    val start2Y: Int,
    /** +15 initial water level (raw; *3/4, clamped to 0x17e0, >>5 = pixel row). */
    val waterLevel: Int,
    /** +17 water speed, added to the level every second frame (113b:2d1c). */
    val waterSpeed: Int,
    /** +19 last byte of the record, not used by the code found so far. */
    val byte19: Int,
)

/** 8 words; the loader copies w0..w6 into parallel arrays and skips w7. */
@Serializable
data class Pad(val words: List<Int>) {
    val w0 get() = words[0]      // -> 290d
    val w1 get() = words[1]      // -> 2921
    val y get() = words[2]       // -> 2935 (converted by 3d4d)
    val w3 get() = words[3]      // -> 2949
    val w4 get() = words[4]      // -> 295d
    val x get() = words[5]       // -> 2971
    val number get() = words[6]  // -> 2985
}

/** Passenger entry: descriptor pointer, pad index, optional route (list of pad + value pairs). */
@Serializable
data class Passenger(val type: Int, val pad: Int, val routePtr: Int, val route: List<RouteStop>)

@Serializable
data class RouteStop(val pad: Int, val value: Int)

/**
 * Object entry. [type] is a DGROUP descriptor pointer that also selects the update handler in 113b:
 * 0x76e4 -> 2a87, 0x7630 -> 2379, 0x76a8 -> 295b, anything else -> 25b1. [words] are the entry words after the type.
 */
@Serializable
data class LevelObject(val type: Int, val handler: Int, val words: List<Int>)

object Levels {
    private const val D = Layout.DGROUP

    fun objectEntryWords(type: Int) = when (type) {
        0x76e4 -> 2
        0x7630 -> 3
        0x76a8 -> 4
        else -> 3
    }

    fun objectHandler(type: Int) = when (type) {
        0x76e4 -> 0x2a87
        0x7630 -> 0x2379
        0x76a8 -> 0x295b
        else -> 0x25b1
    }

    fun extract(exe: Exe): LevelSet {
        val maps = Ice.unpack(exe.from(Layout.SEG_LEVEL_MAPS))
        val onePlayer = recordList(exe, Layout.LEVELS_ONE_PLAYER)
        val team = recordList(exe, Layout.LEVELS_TEAM)
        val passwords1 = passwords(exe, Layout.PASSWORDS_ONE_PLAYER)
        val passwordsTeam = passwords(exe, Layout.PASSWORDS_TEAM)
        check(passwords1.size == onePlayer.size) { "1P passwords ${passwords1.size} != levels ${onePlayer.size}" }
        check(passwordsTeam.size == team.size) { "team passwords ${passwordsTeam.size} != levels ${team.size}" }
        val levels = (onePlayer + team).distinct().sorted().map { level(exe, maps, it) }
        return LevelSet(onePlayer, team, passwords1, passwordsTeam, levels)
    }

    private fun recordList(exe: Exe, table: Int): List<Int> =
        generateSequence(0) { it + 1 }.map { exe.u16(D, table + 2 * it) }.takeWhile { it != 0xffff }.toList()

    private fun passwords(exe: Exe, start: Int): List<String> {
        val result = mutableListOf<String>()
        var p = start
        val sb = StringBuilder()
        while (true) {
            val c = exe.u8(D, p++)
            when (c) {
                0xff -> break
                0 -> { result += sb.toString(); sb.clear() }
                else -> sb.append(c.toChar())
            }
        }
        check(sb.isEmpty()) { "password list at ${start.hex()} not terminated after a 0" }
        return result
    }

    private fun level(exe: Exe, maps: ByteArray, rec: Int): Level {
        fun w(o: Int) = exe.s16(D, o)
        val mapOffset = exe.u16(D, rec)
        val tiles = (0 until Layout.MAP_HEIGHT).map { r ->
            (0 until Layout.MAP_WIDTH).map { c -> maps[mapOffset + r * Layout.MAP_WIDTH + c].toInt() and 0xff }
        }

        // list A: pads, 8 words each, terminated by a negative first word (113b:3a6d)
        val aStart = exe.u16(D, rec + 2)
        var p = aStart
        val pads = mutableListOf<Pad>()
        while (w(p) >= 0) { pads += Pad((0 until 8).map { w(p + 2 * it) }); p += 16 }
        val aEnd = p + 2

        // list B: passengers, 3 words each, terminated by 0xffff (113b:3aaf)
        val bStart = exe.u16(D, rec + 4)
        p = bStart
        val passengers = mutableListOf<Passenger>()
        val routeBytes = sortedMapOf<Int, Int>()
        while (exe.u16(D, p) != 0xffff) {
            val routePtr = exe.u16(D, p + 4)
            val route = mutableListOf<RouteStop>()
            if (routePtr != 0) {
                var q = routePtr
                while (exe.u16(D, q) != 0xffff) { route += RouteStop(w(q), w(q + 2)); q += 4 }
                routeBytes[routePtr] = q + 2
            }
            passengers += Passenger(exe.u16(D, p), w(p + 2), routePtr, route)
            p += 6
        }
        val bEnd = p + 2

        // list C: objects, variable size by type, terminated by 0xffff (113b:3b21)
        val cStart = exe.u16(D, rec + 6)
        p = cStart
        val objects = mutableListOf<LevelObject>()
        while (exe.u16(D, p) != 0xffff) {
            val type = exe.u16(D, p)
            val n = objectEntryWords(type)
            objects += LevelObject(type, objectHandler(type), (1..n).map { w(p + 2 * it) })
            p += 2 * (n + 1)
        }
        val cEnd = p + 2

        val captionPtr = exe.u16(D, rec + 0x0a)
        val captionRaw = generateSequence(captionPtr) { it + 1 }.map { exe.u8(D, it) }.takeWhile { it != 0 }.toList()

        // self-check: pads, routes, passengers, objects and caption must tile the level's data block
        // without gaps or overlaps (their order differs between levels)
        val blocks = (listOf(aStart to aEnd, bStart to bEnd, cStart to cEnd, captionPtr to captionPtr + captionRaw.size + 1) +
            routeBytes.toList()).sortedBy { it.first }
        for ((prev, next) in blocks.zipWithNext()) {
            check(prev.second == next.first) { "level ${rec.hex()}: block ${prev.first.hex()}..${prev.second.hex()} is followed by ${next.first.hex()}" }
        }
        check(pads.size <= 10 && passengers.size <= 16 && objects.size <= 5) { "level ${rec.hex()}: list too long" }

        return Level(
            record = rec,
            mapOffset = mapOffset,
            tiles = tiles,
            pads = pads,
            passengers = passengers,
            objects = objects,
            field08 = w(rec + 0x08),
            captionPtr = captionPtr,
            captionRaw = captionRaw,
            caption = captionRaw.filter { it != 0xfe }.joinToString("") { if (it == 0x0d) "\n" else it.toChar().toString() },
            flag0c = exe.u8(D, rec + 0x0c),
            start1X = w(rec + 0x0d),
            start1Y = w(rec + 0x0f),
            start2X = w(rec + 0x11),
            start2Y = w(rec + 0x13),
            waterLevel = w(rec + 0x15),
            waterSpeed = w(rec + 0x17),
            byte19 = exe.u8(D, rec + 0x19),
        )
    }
}

fun Int.hex() = "0x" + toString(16).padStart(4, '0')
