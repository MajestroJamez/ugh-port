package ugh.verify.replay

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.Game
import ugh.core.game.GameEnd
import ugh.core.game.playGame
import ugh.core.hw.Memory
import ugh.extractor.Exe
import ugh.extractor.Levels
import ugh.extractor.Masks
import ugh.oracle.OriginalUgh
import ugh.verify.CheatPilot
import ugh.verify.Lockstep
import ugh.verify.Pilot
import java.io.File
import java.security.MessageDigest
import java.util.concurrent.Executors

/**
 * Records golden replays (format "UGR 1", see [ReplayWriter]) in lockstep with the original: every level of
 * both modes entered by its password (F2 in the menu), driven by input-only pilots or the cheating pilot (its
 * memory writes are recorded as injections), so a reimplementation can replay the keys and compare the state
 * tick by tick. Every frame also checks that the projection of the original's memory equals the port's.
 */
class GoldenReplayTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val exeSha256 = MessageDigest.getInstance("SHA-256").digest(exe).joinToString("") { "%02x".format(it) }
    private val levels = Levels.extract(Exe(exe))
    private val outDir = File(System.getProperty("ugh.replays"))

    /** The collision mask of each level (by its record), as the extractor computes it for the game data. */
    private val masks = Masks.forLevels(Exe(exe))

    /** [level] 0-based; [difficulty]: 0 easy, 1 medium (the menu's default), 2 hard. */
    data class Spec(val name: String, val seed: Long, val frames: Long, val level: Int = 0, val team: Boolean = false,
                    val difficulty: Int = 1, val cheat: Boolean = false)

    data class Result(val spec: Spec, val file: File, val frames: Long, val end: GameEnd?, val problems: List<String>,
                      val states: Set<String>, val levelsDone: Int, val injections: Int, val summary: String,
                      val audit: StateAudit, val maskRecords: Set<Int>, val probe: ProbeStats)

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
        // the projection of the memory: the audit of the state's completeness watches what it reads (StateAudit)
        val original = StateProjection(audit.reading { seg, off -> ugh.machine.read16(seg, off) })
        val semanticOriginal = SemanticProjection { seg, off -> ugh.machine.read16(seg, off) }
        val semanticPort = SemanticProjection { seg, off -> lockstep.portMem.read16(seg, off) }
        val meta = linkedMapOf(
            "exe-sha256" to exeSha256, "level" to "$level", "players" to if (team) "2" else "1",
            "difficulty" to listOf("easy", "medium", "hard")[difficulty],
            "pilot" to if (cheat) "cheat-pilot" else "random-keys", "seed" to "$seed",
        )
        val file = File(outDir, "$name.ugr")
        var semanticMismatches = 0
        var firstSemanticMismatch: String? = null
        val semanticProblems = LinkedHashSet<String>()
        val unknownStates = ArrayList<String>()
        var end: GameEnd? = null
        val states = HashSet<String>()
        // state handlers that ran (inside a frame: most "Start..." handlers hand over before the frame ends)
        val handlers = StateProjection.PASSENGER_STATES.map { (a, n) -> a to "passenger.$n" } +
            StateProjection.OBJECT_STATES.map { (a, n) -> a to "object.$n" } +
            StateProjection.BONUS_STATES.map { (a, n) -> a to "bonus.$n" }
        for ((address, state) in handlers) ugh.machine.onExecute(OriginalUgh.SEG_GAME, address) { states += state }
        var levelsDone = 0
        var injections = 0
        val maskRecords = HashSet<Int>()
        val probe = ProbeStats().also { it.attachTo(ugh) }
        var lastPhase = ""
        ReplayWriter(file, meta).use { writer ->
            writer.tick(0, emptyList(), semanticOriginal.project("start"))
            val keys = ArrayList<Int>()
            var pending: Map<String, String>? = null
            var pendingInject = emptyMap<String, String>()
            lockstep.onKey += { keys += it }
            // runs first at every boundary: the previous frame's keys are complete, the new frame just ended
            lockstep.atBoundary.add(0) {
                pending?.let {
                    writer.tick(writer.ticks, keys.toList(), it)
                    writer.inject(pendingInject)
                    if (pendingInject.isNotEmpty()) injections++
                }
                pendingInject = emptyMap()
                keys.clear()
                val phase = StateProjection.phaseOfPort()
                // the collision mask: entering the level play and every 64th frame (bit 7 of the background never changes)
                if (phase == "play" && (lastPhase != "play" || lockstep.frames % 64 == 0L)) {
                    val record = currentRecord(lockstep.portMem)
                    maskRecords += record
                    maskProblem(lockstep.portMem, record)?.let { problems += "frame ${lockstep.frames} ($lastPhase -> $phase): $it" }
                }
                lastPhase = phase
                audit.frame(lockstep.portMem.ram, ugh.machine.cpu.ss shl 4, phase)
                for ((k, v) in original.project(phase)) {
                    if (k.startsWith("object.") && k.endsWith(".kind")) states += "object.$v"
                    if (v.startsWith("?")) unknownStates += "frame ${lockstep.frames}: $k=$v"
                }
                val so = semanticOriginal.project(phase)
                val sp = semanticPort.project(phase)
                for ((k, v) in so) {
                    if (v.startsWith("?")) semanticProblems += "unnamed value $k=$v"
                    if (v.startsWith("0x")) semanticProblems += "address in $k=$v"
                }
                if (so != sp) {
                    semanticMismatches++
                    if (firstSemanticMismatch == null) firstSemanticMismatch = "frame ${lockstep.frames}: " +
                        so.keys.union(sp.keys).filter { so[it] != sp[it] }.joinToString { "$it original ${so[it]} port ${sp[it]}" }
                }
                if (so["game.levelDone"] == "1" && pending?.get("game.levelDone") == "0") levelsDone++
                pending = so
            }
            // runs last at every boundary: what the pilots poked into the state is an injection
            lockstep.atBoundary += {
                val before = pending
                if (before != null && cheat) {
                    val after = semanticOriginal.project(before.getValue("game.phase"))
                    val injected = after.filter { before[it.key] != it.value } + before.keys.filter { it !in after }.associateWith { "~" }
                    for (k in injected.keys) if (!SemanticProjection.INJECTABLE.matches(k)) semanticProblems += "injection of $k"
                    if (injected.isNotEmpty()) pendingInject = injected
                }
            }
            lockstep.run { end = playGame() }
            pending?.let { writer.tick(writer.ticks, keys.toList(), it) }
        }
        check(lockstep.mismatchCount == 0) { "${lockstep.mismatchCount} lockstep mismatches: ${lockstep.mismatches.firstOrNull()}" }
        check(unknownStates.isEmpty()) { "state handlers without a name: ${unknownStates.take(3)}" }
        check(semanticMismatches == 0) { "$semanticMismatches semantic projection mismatches, first $firstSemanticMismatch" }
        check(semanticProblems.isEmpty()) { "semantic state: ${semanticProblems.take(5)}" }
        check(probe.outsideSolid == 0L) { "collision probe found solid pixels outside the background page: ${probe.outsideSolid} in rows ${probe.solidRows}" }
        problems += readBackProblems(file)
        val summary = "replay $name: ${lockstep.frames} frames, end $end, levels done $levelsDone, " +
            "injections $injections, ${file.length() / 1024} kB" + (cheatPilot?.let { ", pilot ${it.actions}" } ?: "")
        return Result(spec, file, lockstep.frames, end, problems.map { "$name: $it" }, states, levelsDone, injections, summary, audit, maskRecords, probe)
    }

    /** DGROUP offset of the record of the current level (113b:3976). */
    private fun currentRecord(mem: Memory): Int {
        val table = mem.read16(Game.DGROUP, 0x2909 + ((mem.read16(Game.DGROUP, 0x2634) - 1) shl 1))
        return mem.read16(Game.DGROUP, table + (mem.read16(Game.DGROUP, 0x261c) shl 1))
    }

    /** Bit 7 of the background page of the port's VGA against the exported collision mask of the level. */
    private fun maskProblem(mem: Memory, record: Int): String? {
        val mask = masks[record] ?: return "no collision mask for level record 0x%04x".format(record)
        val page = mem.read16(Game.CODE, Game.CS_BACK_PAGE)
        var differences = 0
        var first: String? = null
        for (y in 0 until Masks.HEIGHT) for (x in 0 until Masks.WIDTH) {
            val solid = mem.vga.planes[x and 3][(page + y * 0x60 + (x shr 2)) and 0xffff].toInt() and 0x80 != 0
            if (solid != Masks.bit(mask, x, y)) { differences++; if (first == null) first = "x $x y $y: page $solid" }
        }
        return if (differences == 0) null else "collision mask of level record 0x%04x: %d pixels differ, first %s".format(record, differences, first)
    }

    /**
     * 113b:1457 in the original: how often the 10 probed bytes lie outside the background page (copter at the top
     * or bottom edge: the neighbouring VGA memory is read) and whether one of them was solid there.
     */
    class ProbeStats {
        var probes = 0L; var outside = 0L; var outsideSolid = 0L
        /** Solid bytes outside the page by row relative to the page (negative = above it). */
        val solidRows = java.util.TreeMap<Int, Int>()

        fun attachTo(ugh: OriginalUgh) {
            val m = ugh.machine
            m.onExecute(OriginalUgh.SEG_GAME, 0x1457) {
                probes++
                val page = m.read16(OriginalUgh.SEG_GAME, Game.CS_BACK_PAGE)
                for (k in PROBE_OFFSETS) {
                    val a = (m.cpu.si + k) and 0xffff
                    if (a - page !in 0 until 0x4800) {
                        outside++
                        // reading loads the latches, as the probe itself does right after
                        if (m.memory.read8(m.cpu.es, a) and 0x80 != 0) { outsideSolid++; solidRows.merge(Math.floorDiv(a - page, 0x60), 1, Int::plus) }
                    }
                }
            }
        }

        fun add(o: ProbeStats) { probes += o.probes; outside += o.outside; outsideSolid += o.outsideSolid; o.solidRows.forEach { (k, v) -> solidRows.merge(k, v, Int::plus) } }

        companion object { private val PROBE_OFFSETS = intArrayOf(0, 3, 5, 0x720, 0x723, 0x725, 0x240, 0x245, 0x480, 0x485) }
    }

    /** The file reads back to the same full states (delta encoding, removed fields). */
    private fun readBackProblems(file: File): List<String> {
        val (meta, ticks) = ReplayReader.read(file)
        val problems = ArrayList<String>()
        if (meta["level"] == null) problems += "meta without level"
        if (ticks.size <= 100) problems += "ticks: ${ticks.size}"
        if (ticks.map { it.tick } != (0L until ticks.size).toList()) problems += "tick numbers not consecutive"
        if (!ticks.all { it.state["game.level"] != null && it.state["game.phase"] != null }) problems += "tick without level / phase"
        val play = ticks.filter { it.state["game.phase"] == "play" }
        if (play.size <= 100 || !play.all { it.state["copter.0.x"] != null }) problems += "play ticks: ${play.size}"
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
        val probe = ProbeStats().also { p -> results.forEach { p.add(it.probe) } }
        val maskRecords = results.flatMap { it.maskRecords }.toSet()
        println("collision probes: ${probe.probes}, bytes outside the background page ${probe.outside}, solid there ${probe.outsideSolid} in rows ${probe.solidRows}")
        println("collision masks checked: ${maskRecords.size} of ${masks.size} levels")
        assertEquals(masks.keys, maskRecords, "levels whose collision mask was checked")
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
