package ugh.verify.replay

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.GameEnd
import ugh.core.game.playGame
import ugh.extractor.Exe
import ugh.extractor.Levels
import ugh.oracle.OriginalUgh
import ugh.verify.CheatPilot
import ugh.verify.Lockstep
import ugh.verify.Pilot
import java.io.File
import java.security.MessageDigest
import java.util.concurrent.Executors

/**
 * Records golden replays (format "UGR 0", see [ReplayWriter]) in lockstep with the original: every level of
 * both modes entered by its password (F2 in the menu), driven by input-only pilots or the cheating pilot (its
 * memory writes are recorded as injections), so a reimplementation can replay the keys and compare the state
 * tick by tick. Every frame also checks that the projection of the original's memory equals the port's.
 */
class GoldenReplayTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val exeSha256 = MessageDigest.getInstance("SHA-256").digest(exe).joinToString("") { "%02x".format(it) }
    private val levels = Levels.extract(Exe(exe))
    private val outDir = File(System.getProperty("ugh.replays"))

    /** [level] 0-based; [difficulty]: 0 easy, 1 medium (the menu's default), 2 hard. */
    data class Spec(val name: String, val seed: Long, val frames: Long, val level: Int = 0, val team: Boolean = false,
                    val difficulty: Int = 1, val cheat: Boolean = false)

    data class Result(val spec: Spec, val file: File, val frames: Long, val end: GameEnd?, val problems: List<String>,
                      val states: Set<String>, val levelsDone: Int, val injections: Int, val summary: String,
                      val audit: StateAudit)

    private fun toNewGame(menuKeys: List<Int>, password: String?): OriginalUgh {
        val ugh = OriginalUgh(exe)
        assertTrue(ugh.runUntil(0x113b, 0x00d8), "menu")
        ugh.runGameFrames(10)
        for (k in menuKeys) { ugh.tap(k); ugh.runGameFrames(10) }
        if (password != null) { ugh.tap(0x3c); ugh.runGameFrames(10); ugh.type(password + "\n"); ugh.runGameFrames(10) }
        ugh.tap(0x3b)
        repeat(20) {
            if (ugh.runUntil(0x113b, 0x0c61, 100)) return ugh
            ugh.tap(0x39)
        }
        error("new game not reached")
    }

    private fun record(spec: Spec): Result {
        val (name, seed, frames, level, team, difficulty, cheat) = spec
        // F4 = team mode, F3 = next difficulty (medium -> hard -> easy)
        val menuKeys = (if (team) listOf(0x3e) else emptyList()) + List((difficulty + 2) % 3) { 0x3d }
        val password = if (level == 0) null else (if (team) levels.passwordsTeam else levels.passwordsOnePlayer)[level]
        val ugh = toNewGame(menuKeys, password)
        val problems = ArrayList<String>()
        fun check(ok: Boolean, problem: () -> String) { if (!ok) problems += problem() }
        check(ugh.dgroup16(0x2638) == difficulty) { "difficulty ${ugh.dgroup16(0x2638)}" }
        check(ugh.dgroup16(0x261c) == level) { "level ${ugh.dgroup16(0x261c)}" }

        val lockstep = Lockstep(ugh, maxFrames = frames)
        val cheatPilot = if (cheat) CheatPilot(ugh, seed).also { it.attachTo(lockstep) } else null
        if (!cheat) Pilot(ugh, seed, player = 0).attachTo(lockstep)
        if (team) Pilot(ugh, seed + 1000, player = 1).attachTo(lockstep)

        val audit = StateAudit()
        val original = StateProjection(audit.reading { seg, off -> ugh.machine.read16(seg, off) })
        val port = StateProjection { seg, off -> lockstep.portMem.read16(seg, off) }
        val meta = linkedMapOf(
            "exe-sha256" to exeSha256, "start" to "newgame", "level" to "$level", "password" to (password ?: "NONE"),
            "players" to if (team) "2" else "1", "difficulty" to "$difficulty",
            "pilot" to if (cheat) "cheat-pilot" else "random-keys", "seed" to "$seed",
        )
        val file = File(outDir, "$name.ugr")
        var projectionMismatches = 0
        val unknownStates = ArrayList<String>()
        var firstMismatch: String? = null
        var end: GameEnd? = null
        val states = HashSet<String>()
        // state handlers that ran (inside a frame: most "Start..." handlers hand over before the frame ends)
        val handlers = StateProjection.PASSENGER_STATES.map { (a, n) -> a to "passenger.$n" } +
            StateProjection.OBJECT_STATES.map { (a, n) -> a to "object.$n" } +
            StateProjection.BONUS_STATES.map { (a, n) -> a to "bonus.$n" }
        for ((address, state) in handlers) ugh.machine.onExecute(OriginalUgh.SEG_GAME, address) { states += state }
        var levelsDone = 0
        var injections = 0
        ReplayWriter(file, meta).use { writer ->
            writer.tick(0, 0, emptyList(), original.project("start"))
            val keys = ArrayList<Int>()
            var pending: Map<String, String>? = null
            var pendingWait = 0
            var pendingInject = emptyMap<String, String>()
            lockstep.onKey += { keys += it }
            // runs first at every boundary: the previous frame's keys are complete, the new frame just ended
            lockstep.atBoundary.add(0) {
                pending?.let {
                    writer.tick(writer.ticks, pendingWait, keys.toList(), it)
                    writer.inject(pendingInject)
                    if (pendingInject.isNotEmpty()) injections++
                }
                pendingInject = emptyMap()
                keys.clear()
                val phase = StateProjection.phaseOfPort()
                audit.frame(lockstep.portMem.ram, ugh.machine.cpu.ss shl 4, phase)
                val o = original.project(phase)
                val p = port.project(phase)
                for ((k, v) in o) {
                    if (k.startsWith("object.") && k.endsWith(".kind")) states += "object.$v"
                    if (v.startsWith("?")) unknownStates += "frame ${lockstep.frames}: $k=$v"
                }
                if (o["game.levelDone"] == "1" && pending?.get("game.levelDone") == "0") levelsDone++
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
        check(lockstep.mismatchCount == 0) { "${lockstep.mismatchCount} lockstep mismatches: ${lockstep.mismatches.firstOrNull()}" }
        check(projectionMismatches == 0) { "$projectionMismatches projection mismatches, first $firstMismatch" }
        check(unknownStates.isEmpty()) { "state handlers without a name: ${unknownStates.take(3)}" }
        problems += readBackProblems(file)
        val summary = "replay $name: ${lockstep.frames} frames, end $end, levels done $levelsDone, " +
            "injections $injections, ${file.length() / 1024} kB" + (cheatPilot?.let { ", pilot ${it.actions}" } ?: "")
        return Result(spec, file, lockstep.frames, end, problems.map { "$name: $it" }, states, levelsDone, injections, summary, audit)
    }

    /** The file reads back to the same full states (delta encoding, removed fields). */
    private fun readBackProblems(file: File): List<String> {
        val (meta, ticks) = ReplayReader.read(file)
        val problems = ArrayList<String>()
        if (meta["start"] != "newgame") problems += "meta start ${meta["start"]}"
        if (ticks.size <= 100) problems += "ticks: ${ticks.size}"
        if (ticks.map { it.tick } != (0L until ticks.size).toList()) problems += "tick numbers not consecutive"
        if (!ticks.all { it.state["game.level"] != null && it.state["game.phase"] != null }) problems += "tick without level / phase"
        val play = ticks.filter { it.state["game.phase"] == "play" }
        if (play.size <= 100 || !play.all { it.state["copter.0.xf"] != null }) problems += "play ticks: ${play.size}"
        return problems
    }

    /**
     * The set: every level of both modes with the cheating pilot (passengers, enemies, bonus items), a few
     * long ones in which the cheating pilot completes levels, and random pilots until game over on all
     * difficulties, starting from different levels.
     */
    private fun specs(): List<Spec> = buildList {
        for (l in levels.passwordsOnePlayer.indices) add(Spec("1p-L%02d-cheat".format(l + 1), seed = 100L + l, frames = 2000, level = l, cheat = true))
        for (l in levels.passwordsTeam.indices) add(Spec("team-L%02d-cheat".format(l + 1), seed = 200L + l, frames = 2000, level = l, team = true, cheat = true))
        for (l in listOf(0, 17, 40)) add(Spec("1p-L%02d-cheat-long".format(l + 1), seed = 300L + l, frames = 30000, level = l, cheat = true))
        add(Spec("team-L01-cheat-long", seed = 400, frames = 30000, team = true, cheat = true))
        add(Spec("1p-L01-medium-seed11", seed = 11, frames = 40000))
        add(Spec("1p-L01-easy-seed21", seed = 21, frames = 40000, difficulty = 0))
        add(Spec("1p-L01-hard-seed22", seed = 22, frames = 40000, difficulty = 2))
        add(Spec("1p-L25-easy-seed23", seed = 23, frames = 40000, level = 24, difficulty = 0))
        add(Spec("1p-L60-hard-seed24", seed = 24, frames = 40000, level = 59, difficulty = 2))
        add(Spec("team-L01-medium-seed3", seed = 3, frames = 40000, team = true))
        add(Spec("team-L50-hard-seed4", seed = 4, frames = 40000, level = 49, team = true, difficulty = 2))
    }

    @Test
    fun `golden replays of every level`() {
        assertEquals(69, levels.passwordsOnePlayer.size, "one player levels")
        assertEquals(81, levels.passwordsTeam.size, "team levels")
        outDir.mkdirs()
        val pool = Executors.newFixedThreadPool((Runtime.getRuntime().availableProcessors() - 2).coerceIn(1, 12))
        val results = try {
            specs().map { spec -> pool.submit<Result> { record(spec) } }.map { it.get() }
        } finally {
            pool.shutdown()
        }
        results.forEach { println(it.summary) }

        val problems = results.flatMap { it.problems }
        problems.forEach(::println)
        assertEquals(emptyList<String>(), problems.take(10), "${problems.size} problems")

        val seen = results.flatMap { it.states }.toSet()
        val named = StateProjection.PASSENGER_STATES.values.map { "passenger.$it" } +
            StateProjection.OBJECT_STATES.values.map { "object.$it" } + StateProjection.OBJECT_KINDS.values.map { "object.$it" } +
            StateProjection.BONUS_STATES.values.map { "bonus.$it" }
        val missing = named.toSet() - seen - UNREACHABLE
        val completed = results.filter { it.levelsDone > 0 }
        println("${results.size} replays, ${results.sumOf { it.frames }} frames, ${results.sumOf { it.file.length() } / 1024 / 1024} MB")
        println("levels completed: ${completed.sumOf { it.levelsDone }} in ${completed.map { it.spec.name }}")
        println("named states never run: ${missing.sorted()}")
        val audit = StateAudit().also { a -> results.forEach { a.merge(it.audit) } }
        val unprojected = audit.unprojected(StateProjection.NOT_PROJECTED)
        File(outDir, "audit.txt").writeText((audit.unprojected().map { it.toString() + (StateProjection.NOT_PROJECTED
            .firstOrNull { h -> it.linear in h.linear until h.linear + h.length }?.let { h -> " - not projected: ${h.reason}" } ?: "") })
            .joinToString("\n", postfix = "\n"))
        println("changed, not projected and not explained: ${unprojected.size} ranges")
        unprojected.forEach { println("  $it") }
        println("not projected, never changed: ${audit.unchanged(StateProjection.NOT_PROJECTED).map { StateAudit.where(it.linear) }}")
        assertTrue(completed.size >= 3, "levels completed in ${completed.size} replays")
        assertEquals(emptyList<String>(), unprojected.map { it.toString() }, "state the projection misses")
        assertEquals(emptyList<String>(), missing.sorted(), "named states never run")
    }

    companion object {
        /**
         * Handlers no state transition reaches:
         * - passenger Idle (2195) is only in slots +24 / +26 of the water set descriptors (776a, 77fe, 7892), which
         *   are entered from Riding / Hanging; a passenger of the water set never rides (boarding 19e0 switches
         *   back to the normal set) or hangs.
         * - walker Recovering2 (28a3) is only in slot +1a of the walker descriptor (766c); both recovery starts
         *   (2830, 288f) continue with slot +16 (2844).
         */
        val UNREACHABLE = setOf("passenger.Idle", "object.Recovering2")
    }
}
