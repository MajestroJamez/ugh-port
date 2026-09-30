package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.extractor.Exe
import ugh.extractor.Levels
import ugh.oracle.OriginalUgh
import java.io.File

/** Whole frames of level play (logic + drawing + VGA state) against the original. */
class FrameTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val levels = Levels.extract(Exe(exe))

    private fun run(level: Int, seed: Long, frames: Long, cheat: Boolean, team: Boolean = false) {
        val ugh = OriginalUgh(exe)
        val password = if (level == 0) null else if (team) levels.passwordsTeam[level] else levels.passwordsOnePlayer[level]
        assertTrue(ugh.startGame(menuKeys = if (team) listOf(0x3e) else emptyList(), password = password))
        val diff = FrameDiff(ugh)
        if (cheat) CheatPilot(ugh, seed).attachTo(diff) else Pilot(ugh, seed).attach()
        if (team) Pilot(ugh, seed + 100, player = 1).attach()
        ugh.runGameFrames(frames)
        println("${if (team) "team " else ""}level ${level + 1} seed $seed ${if (cheat) "cheat" else "pilot"}: " +
            "${diff.frames} frames compared, ${diff.mismatchCount} mismatches, not compared: ${diff.special}")
        diff.mismatches.forEach(::println)
        assertTrue(diff.frames > frames / 3, "too few frames compared")
        assertEquals(0, diff.mismatchCount)
    }

    @Test
    fun `level 1 frames match the original`() = run(level = 0, seed = 1, frames = 2000, cheat = false)

    @Test
    fun `frame harness detects a wrong pixel and a wrong colour`() {
        for ((what, corrupt) in listOf<Pair<String, (ugh.core.game.Game) -> Unit>>(
            "pixel" to { g -> g.mem.vga.planes[1][0x1234] = (g.mem.vga.planes[1][0x1234] + 1).toByte() },
            "colour" to { g -> g.mem.vga.dac[100] = (g.mem.vga.dac[100] + 1) and 0x3f },
        )) {
            val ugh = OriginalUgh(exe)
            assertTrue(ugh.startGame())
            val diff = FrameDiff(ugh, afterPortFrame = corrupt)
            ugh.runGameFrames(50)
            println("corrupted $what: ${diff.mismatchCount} of ${diff.frames} frames reported, first: ${diff.mismatches.firstOrNull()}")
            assertEquals(diff.frames, diff.mismatchCount)
        }
    }

    @Test
    fun `frames with passengers and enemies match the original`() {
        run(level = 0, seed = 2, frames = 6000, cheat = true)
        run(level = 4, seed = 3, frames = 4000, cheat = true)
        run(level = 3, seed = 4, frames = 4000, cheat = true)
    }

    @Test
    fun `team mode frames match the original`() = run(level = 1, seed = 5, frames = 3000, cheat = false, team = true)

    @Test
    fun `rain frames match the original`() {
        val rainy = levels.onePlayer.indexOfFirst { rec -> levels.levels.first { it.record == rec }.flag0c != 0 }
        println("first level with rain: ${rainy + 1}")
        run(level = rainy, seed = 6, frames = 3000, cheat = true)
    }
}
