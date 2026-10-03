package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import ugh.core.game.Game
import ugh.core.game.Game.Companion.s16
import ugh.core.game.loadLevel
import ugh.core.hw.Memory
import ugh.core.hw.MzLoader
import ugh.extractor.Exe
import ugh.extractor.Levels
import ugh.extractor.LogicData
import ugh.extractor.Masks
import ugh.extractor.Names
import ugh.extractor.UgdRecord
import java.io.File

/**
 * The game data for the C++ logic (format UGD 1, [LogicData]) against the port: every level of both modes is loaded
 * by the port's level load (113b:3976) and what it put into memory - pads, copter starts, water, wind, passengers,
 * routes, enemies - must be what the data file says, already converted. The masks must be the background pages
 * (the 320 px of the screen; the page is 384 px wide and empty beyond).
 */
class LogicDataTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val levelSet = Levels.extract(Exe(exe))
    private val file = File(System.getProperty("ugh.out"), "logic/ugh-data.ugd").also { LogicData.write(Exe(exe), it) }
    private val records = LogicData.read(file)

    /** The records of each level: its `level` record and the ones after it up to the next level. */
    private val levels: List<List<UgdRecord>> = buildList {
        var current: MutableList<UgdRecord>? = null
        for (r in records) {
            if (r.type == "level") current = mutableListOf<UgdRecord>().also { add(it) }
            else if (r.type == "order") current = null
            current?.add(r)
        }
    }

    private fun order(mode: String) = records.first { it.type == "order" && mode in it.fields }.ints(mode)

    @Test
    fun `the data of every level is what the level load computes`() {
        assertEquals(levelSet.levels.size, levels.size, "levels")
        var checked = 0
        for ((players, mode) in listOf(1 to "oneplayer", 2 to "team")) {
            for ((number, id) in order(mode).withIndex()) {
                val game = Game(Memory().also { MzLoader.load(exe, it) })
                game.setD(0x2634, players)
                game.setD(0x261c, number)
                game.loadLevel()
                val problems = compare(game, levels[id])
                assertEquals(emptyList<String>(), problems, "level $number of $mode (data level $id)")
                checked++
            }
        }
        assertEquals(69 + 81, checked)
    }

    @Test
    fun `the masks are the background pages`() {
        val masks = Masks.forLevels(Exe(exe))
        for ((id, level) in levelSet.levels.withIndex()) {
            val rows = levels[id].filter { it.type == "mask" }.map { it.name!! }
            assertEquals(192, rows.size, "mask rows of level $id")
            val mask = masks.getValue(level.record)
            for (y in 0 until Masks.HEIGHT) for (x in 0 until Masks.WIDTH) {
                val solid = x < LogicData.MASK_WIDTH && (rows[y].substring(x / 4, x / 4 + 1).toInt(16) and (8 shr (x and 3))) != 0
                assertEquals(Masks.bit(mask, x, y), solid, "level $id pixel $x, $y")
            }
        }
    }

    private fun compare(g: Game, level: List<UgdRecord>): List<String> {
        val problems = ArrayList<String>()
        fun check(what: String, expected: Any, actual: Any) { if (expected.toString() != actual.toString()) problems += "$what: data $actual, memory $expected" }
        val head = level.first()
        check("toDeliver", g.d8(0x28f1), head["toDeliver"])
        check("wind", listOf("none", "left", "right")[g.d8(Game.V_WIND)], head["wind"])
        check("start0", "${g.d(Game.P_XF)},${g.d(Game.P_YF)}", head["start0"])
        check("start1", "${g.d(Game.P_XF + 2)},${g.d(Game.P_YF + 2)}", head["start1"])
        check("water", g.d(0x28fe), head["water"])
        check("waterSpeed", g.d(0x2900), head["waterSpeed"])

        val pads = level.filter { it.type == "pad" }
        val padCount = (0 until 10).takeWhile { g.d(Game.PAD_LEFT + 2 * it) >= 0 }.size
        check("pads", padCount, pads.size)
        for ((i, p) in pads.withIndex()) {
            val si = 2 * i
            check("pad $i", listOf(g.d(Game.PAD_LEFT + si), g.d(Game.PAD_RIGHT + si), g.d(Game.PAD_Y + si), g.d(0x2949 + si),
                g.d(0x295d + si), g.d(0x2971 + si), g.d(0x2985 + si)),
                listOf(p["left"], p["right"], p["y"], p["door"], p["wait"], p["stand"], p["number"]))
        }

        val passengers = level.filter { it.type == "routePassenger" || it.type == "standingPassenger" }
        val passengerCount = (0 until 16).takeWhile { g.u(0x29ad + 2 * it) != 0xffff }.size
        check("passengers", passengerCount, passengers.size)
        for ((i, p) in passengers.withIndex()) {
            val si = 2 * i
            val kind = g.u(0x29ad + si)
            if (p.type == "standingPassenger") {
                check("passenger $i kind", Names.passengerKind(kind), "standing")
                check("passenger $i position", "${g.d(0x2aed + si)},${g.d(0x2b0d + si)}", "${p["x"]},${p["y"]}")
            } else {
                check("passenger $i kind", Names.passengerKind(kind), p["kind"])
                // the route as the original reads it at every stop (113b:149c: delay * 3/2)
                val stops = ArrayList<String>()
                var a = g.u(0x29ed + si)
                while (g.u(a + 4) != 0xffff) { stops += "${g.d(a)}/${s16(g.times3half(g.u(a + 2)))}"; a += 4 }
                stops += "${g.d(a)}"
                check("passenger $i route", stops.joinToString(","), p["route"])
            }
        }

        val enemies = level.filter { it.type in setOf("flyer", "walker", "blower", "tree") }
        val enemyCount = (0 until 5).takeWhile { g.u(0x2cad + 2 * it) != 0xffff }.size
        check("enemies", enemyCount, enemies.size)
        for ((i, e) in enemies.withIndex()) {
            val si = 2 * i
            check("enemy $i kind", Names.enemyKind(g.u(0x2cad + si)), e.type)
            val position = "${g.d(0x2cc1 + si)},${g.d(0x2ccb + si)}"
            when (e.type) {
                "flyer" -> check("flyer $i", "${g.d(0x2cdf + si)},${g.d(0x2ce9 + si)}", "${e["startDelay"]},${e["speed"]}")
                "walker" -> check("walker $i", "${g.d(0x2cb7 + si)},$position,${g.d(0x2ce9 + si)}", "${e["pad"]},${e["x"]},${e["y"]},${e["speed"]}")
                "blower" -> check("blower $i", position, "${e["x"]},${e["y"]}")
                "tree" -> {
                    val drops = ArrayList<String>()
                    var a = g.u(0x2cd5 + si)
                    while (g.u(a) != 0xffff) { drops += Names.bonusKind(g.u(a)); a += 2 }
                    check("tree $i", "$position,${drops.joinToString(",").ifEmpty { "-" }}", "${e["x"]},${e["y"]},${e["drops"]}")
                }
            }
        }
        return problems
    }
}
