package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.Regs
import ugh.core.game.objectsUpdate
import ugh.extractor.Exe
import ugh.extractor.Levels
import ugh.oracle.OriginalUgh
import java.io.File

/**
 * Enemies and other active objects (113b:2363) against the original. Levels with each object type are
 * picked from the level data and entered by password (read from the EXE at test time).
 */
class ObjectsTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val levels = Levels.extract(Exe(exe))

    private fun regs(r: CallDiff.Regs) = Regs(r.ax, r.bx, r.cx, r.dx, r.si, r.di, r.bp)

    private fun run(level: Int, seed: Long, frames: Long): Set<Int> {
        val ugh = OriginalUgh(exe)
        val password = if (level == 0) null else levels.passwordsOnePlayer[level]
        assertTrue(ugh.startGame(password = password))
        val objects = CallDiff(ugh, "objectsUpdate", 0x2363, { g, r -> g.objectsUpdate(regs(r)) })
        CheatPilot(ugh, seed).attach()
        val states = sortedSetOf<Int>()
        val played = sortedSetOf<Int>()
        objects.onCall { g ->
            played += g.u(0x261c)
            var bx = 0
            while (g.u(0x2cad + bx) != 0xffff && bx < 10) { states += g.u(0x2cf3 + bx); bx += 2 }
        }
        ugh.runGameFrames(frames)
        println("level ${level + 1} seed $seed: ${objects.calls} calls / ${objects.mismatchCount} mismatches, " +
            "played ${played.map { it + 1 }}, states ${states.joinToString(" ") { "%04x".format(it) }}")
        objects.mismatches.forEach(::println)
        assertTrue(objects.calls > 500, "too few calls")
        assertEquals(0, objects.mismatchCount)
        return states
    }

    @Test
    fun `objects match the original`() {
        val seen = sortedSetOf<Int>()
        // for every object type, the first two one-player levels containing it
        for (type in listOf(0x7630, 0x766c, 0x76a8, 0x76e4)) {
            val withType = levels.onePlayer.withIndex()
                .filter { (_, rec) -> levels.levels.first { it.record == rec }.objects.any { it.type == type } }
                .map { it.index }.take(2)
            for ((i, level) in withType.withIndex()) seen += run(level, seed = 10L * type + i, frames = 8000)
        }
        println("all object states seen: ${seen.joinToString(" ") { "%04x".format(it) }}")
    }

    @Test
    fun `enemies hit by dropped passengers match the original`() {
        val seen = sortedSetOf<Int>()
        // only the standing passenger (type 78dc) can be dropped
        for (type in listOf(0x7630, 0x76a8, 0x766c)) {
            val candidates = levels.onePlayer.withIndex().filter { (_, rec) ->
                val l = levels.levels.first { it.record == rec }
                l.objects.any { it.type == type } && l.passengers.any { it.type == 0x78dc }
            }.map { it.index }
            println("levels with %04x and a standing passenger: %s".format(type, candidates.map { it + 1 }))
            for ((i, level) in candidates.take(2).withIndex()) for (seed in 1L..2L) seen += run(level, seed = 1000L * type + 10 * i + seed, frames = 10000)
        }
        println("states seen: ${seen.joinToString(" ") { "%04x".format(it) }}")
    }
}
