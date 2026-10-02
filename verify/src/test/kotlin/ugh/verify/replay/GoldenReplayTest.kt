package ugh.verify.replay

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.GameEnd
import ugh.core.game.playGame
import ugh.oracle.OriginalUgh
import ugh.verify.CheatPilot
import ugh.verify.Lockstep
import ugh.verify.Pilot
import java.io.File
import java.security.MessageDigest

/**
 * Records golden replays (format "UGR 0", see [ReplayWriter]) in lockstep with the original: input-only
 * pilots (no memory pokes), so a reimplementation can replay the keys and compare the state tick by tick.
 * Every frame also checks that the projection of the original's memory equals the port's.
 */
class GoldenReplayTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val outDir = File(System.getProperty("ugh.replays"))

    private fun toNewGame(menuKeys: List<Int>): OriginalUgh {
        val ugh = OriginalUgh(exe)
        assertTrue(ugh.runUntil(0x113b, 0x00d8), "menu")
        ugh.runGameFrames(10)
        for (k in menuKeys) { ugh.tap(k); ugh.runGameFrames(10) }
        ugh.tap(0x3b)
        repeat(20) {
            if (ugh.runUntil(0x113b, 0x0c61, 100)) return ugh
            ugh.tap(0x39)
        }
        error("new game not reached")
    }

    /** [difficulty]: 0 easy, 1 medium (the menu's default), 2 hard. */
    private fun record(name: String, seed: Long, frames: Long, team: Boolean = false, difficulty: Int = 1,
                       cheat: Boolean = false): File {
        // F4 = team mode, F3 = next difficulty (medium -> hard -> easy)
        val menuKeys = (if (team) listOf(0x3e) else emptyList()) + List((difficulty + 2) % 3) { 0x3d }
        val ugh = toNewGame(menuKeys)
        val lockstep = Lockstep(ugh, maxFrames = frames)
        val cheatPilot = if (cheat) CheatPilot(ugh, seed).also { it.attachTo(lockstep) } else null
        if (!cheat) Pilot(ugh, seed, player = 0).attachTo(lockstep)
        if (team) Pilot(ugh, seed + 1000, player = 1).attachTo(lockstep)

        val original = StateProjection { seg, off -> ugh.machine.read16(seg, off) }
        val port = StateProjection { seg, off -> lockstep.portMem.read16(seg, off) }
        assertEquals(difficulty, ugh.dgroup16(0x2638), "difficulty")
        val meta = linkedMapOf(
            "exe-sha256" to MessageDigest.getInstance("SHA-256").digest(exe).joinToString("") { "%02x".format(it) },
            "start" to "newgame", "players" to if (team) "2" else "1", "difficulty" to "$difficulty",
            "pilot" to if (cheat) "cheat-pilot" else "random-keys", "seed" to "$seed",
        )
        val file = File(outDir, "$name.ugr")
        var projectionMismatches = 0
        val unknownStates = ArrayList<String>()
        var firstMismatch: String? = null
        var end: GameEnd? = null
        ReplayWriter(file, meta).use { writer ->
            writer.tick(0, 0, emptyList(), original.project("start"))
            val keys = ArrayList<Int>()
            var pending: Map<String, String>? = null
            var pendingWait = 0
            var pendingInject = emptyMap<String, String>()
            lockstep.onKey += { keys += it }
            // runs first at every boundary: the previous frame's keys are complete, the new frame just ended
            lockstep.atBoundary.add(0) {
                pending?.let { writer.tick(writer.ticks, pendingWait, keys.toList(), it); writer.inject(pendingInject) }
                pendingInject = emptyMap()
                keys.clear()
                val phase = StateProjection.phaseOfPort()
                val o = original.project(phase)
                val p = port.project(phase)
                o.filter { it.value.startsWith("?") }.forEach { unknownStates += "frame ${lockstep.frames}: ${it.key}=${it.value}" }
                if (o != p) {
                    projectionMismatches++
                    if (firstMismatch == null) firstMismatch = "frame ${lockstep.frames}: " +
                        o.keys.union(p.keys).filter { o[it] != p[it] }.joinToString { "$it original ${o[it]} port ${p[it]}" }
                }
                pending = o
                pendingWait = lockstep.waitingIn()
            }
            // runs last at every boundary: what the pilots poked into the state is an injection
            lockstep.atBoundary += {
                val before = pending
                if (before != null && cheat) {
                    val after = original.project(before.getValue("game.phase"))
                    val injected = after.filter { before[it.key] != it.value }
                    if (injected.isNotEmpty()) pendingInject = injected
                }
            }
            lockstep.run { end = playGame() }
            pending?.let { writer.tick(writer.ticks, pendingWait, keys.toList(), it) }
        }
        println("replay $name: ${lockstep.frames} frames, ${lockstep.mismatchCount} lockstep mismatches, " +
            "$projectionMismatches projection mismatches, end $end, ${file.length() / 1024} kB -> $file" +
            (cheatPilot?.let { ", pilot ${it.actions}" } ?: ""))
        lockstep.mismatches.forEach(::println)
        firstMismatch?.let(::println)
        assertEquals(0, lockstep.mismatchCount)
        assertEquals(0, projectionMismatches)
        assertEquals(emptyList<String>(), unknownStates.take(5), "state handlers without a name")
        return file
    }

    /** The file reads back to the same full states (delta encoding, removed fields). */
    private fun checkReadBack(file: File) {
        val (meta, ticks) = ReplayReader.read(file)
        assertEquals("newgame", meta["start"])
        assertTrue(ticks.size > 100, "ticks: ${ticks.size}")
        assertEquals((0L until ticks.size).toList(), ticks.map { it.tick })
        assertTrue(ticks.all { it.state["game.level"] != null && it.state["game.phase"] != null })
        val play = ticks.filter { it.state["game.phase"] == "play" }
        assertTrue(play.size > 100 && play.all { it.state["copter.0.xf"] != null }, "play ticks: ${play.size}")
        println("  phases: " + ticks.groupingBy { it.state.getValue("game.phase") }.eachCount())
    }

    @Test
    fun `one player, medium, until game over`() = checkReadBack(record("1p-medium-seed11", seed = 11, frames = 40000))

    @Test
    fun `one player, easy and hard`() {
        checkReadBack(record("1p-easy-seed21", seed = 21, frames = 40000, difficulty = 0))
        checkReadBack(record("1p-hard-seed22", seed = 22, frames = 40000, difficulty = 2))
    }

    @Test
    fun `one player with the cheating pilot (passengers, enemies, bonus items)`() {
        val file = record("1p-medium-cheat-seed1", seed = 1, frames = 6000, cheat = true)
        checkReadBack(file)
        val (_, ticks) = ReplayReader.read(file)
        val injections = ticks.count { it.inject.isNotEmpty() }
        val states = ticks.flatMap { t -> t.state.filterKeys { it.endsWith(".state") }.values }.toSet()
        println("  injections: $injections, states seen: ${states.sorted()}")
        assertTrue(injections > 10, "injections: $injections")
    }

    @Test
    fun `team mode, medium`() = checkReadBack(record("team-medium-seed3", seed = 3, frames = 12000, team = true, difficulty = 1))
}
