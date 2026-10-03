package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.Game
import ugh.oracle.OriginalUgh
import java.io.File

class CopterPhysicsTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()

    private fun run(frames: Long, seed: Long, port: (Game, CallDiff.Regs) -> Unit): Pair<CallDiff, Map<String, Int>> {
        val ugh = OriginalUgh(exe)
        assertTrue(ugh.startGame())
        val diff = CallDiff(ugh, "copterUpdate", 0x1095, port)
        Pilot(ugh, seed).attach()
        // what the pilot actually did, so that a green test also means something
        val stats = mutableMapOf("flying" to 0, "landed" to 0, "wallHits" to 0, "fastFalls" to 0)
        var lastLanded = -1
        var landings = 0
        ugh.onGameFrame {
            val landed = ugh.dgroupS16(Game.P_LANDED)
            if (landed < 0) stats.merge("flying", 1, Int::plus) else stats.merge("landed", 1, Int::plus)
            if (landed >= 0 && lastLanded < 0) landings++
            lastLanded = landed
            if (ugh.dgroupS16(Game.P_IMPACT) > 0) stats.merge("wallHits", 1, Int::plus)
            if (ugh.dgroupS16(Game.P_VY) > 0x800) stats.merge("fastFalls", 1, Int::plus)
        }
        ugh.runGameFrames(frames)
        stats["landings"] = landings
        return diff to stats
    }

    @Test
    fun `113b 1095 copter physics matches the original`() {
        for (seed in 1L..3L) {
            val (diff, stats) = run(3000, seed) { g, r -> g.copterUpdate(r.bx) }
            println("seed $seed: ${diff.calls} calls compared, ${diff.mismatchCount} mismatches, $stats")
            diff.mismatches.forEach(::println)
            assertTrue(diff.calls > 1000, "too few calls: ${diff.calls}")
            assertEquals(0, diff.mismatchCount)
        }
    }

    @Test
    fun `differential harness detects a wrong port`() {
        // gravity off by one unit: must be reported
        val (diff, _) = run(300, 1) { g, r -> g.copterUpdate(r.bx); g.addD(Game.P_VY + r.bx, 1) }
        println("broken port: ${diff.mismatchCount} mismatches, first: ${diff.mismatches.firstOrNull()}")
        assertTrue(diff.mismatchCount > 0)
    }
}
