package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.GameEnd
import ugh.core.game.playGame
import ugh.oracle.OriginalUgh
import java.io.File

/** Level start, level play and level end of the port in lockstep with the original. */
class LockstepTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()

    /** Power-on, main menu, F1, skips the opening cut scene; stops at 113b:0c61 (new game). */
    private fun toNewGame(menuKeys: List<Int> = emptyList(), adlib: Boolean = false): OriginalUgh {
        val ugh = OriginalUgh(exe, adlib = adlib)
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

    private fun run(seed: Long, frames: Long, team: Boolean = false, adlib: Boolean = false) {
        val ugh = toNewGame(if (team) listOf(0x3e) else emptyList(), adlib)
        val lockstep = Lockstep(ugh, maxFrames = frames)
        val pilot = CheatPilot(ugh, seed).also { it.attachTo(lockstep) }
        var end: GameEnd? = null
        lockstep.run { end = playGame() }
        println("${if (team) "team" else "1P"} seed $seed: ${lockstep.frames} frames in lockstep, " +
            "${lockstep.mismatchCount} mismatches, level ${ugh.dgroup16(0x261c) + 1}, end $end, ${pilot.actions}" +
            if (adlib) ", AdLib: ${lockstep.adlibWrites} writes, ${lockstep.timerInterrupts} timer interrupts" else "")
        lockstep.mismatches.forEach(::println)
        assertEquals(0, lockstep.mismatchCount)
    }

    @Test
    fun `level start, play and end match the original`() = run(seed = 1, frames = 6000)

    @Test
    fun `crashes, lost lives and game over match the original`() = gameOver(adlib = false)

    @Test
    fun `crashes and game over with an AdLib match the original`() = gameOver(adlib = true)

    private fun gameOver(adlib: Boolean) {
        val ugh = toNewGame(adlib = adlib)
        val lockstep = Lockstep(ugh, maxFrames = 40000)
        Pilot(ugh, seed = 11).attachTo(lockstep)
        val levelStarts = mutableListOf<Int>()
        var lastLives = -1
        lockstep.atBoundary += { val lives = ugh.dgroup8(0x263c); if (lives != lastLives) { levelStarts += lives; lastLives = lives } }
        var end: GameEnd? = null
        lockstep.run { end = playGame() }
        println("game over run (AdLib $adlib): ${lockstep.frames} frames, ${lockstep.mismatchCount} mismatches, end $end, " +
            "lives seen $levelStarts, ${lockstep.adlibWrites} AdLib writes")
        lockstep.mismatches.forEach(::println)
        assertEquals(GameEnd.GAME_OVER, end)
        assertEquals(0, lockstep.mismatchCount)
        // the original has returned from 113b:0c07 to the main menu code as well
        assertTrue(ugh.runUntil(0x113b, 0x0155, 10), "original did not return from the game")
    }

    @Test
    fun `several levels in lockstep`() {
        run(seed = 2, frames = 20000)
        run(seed = 3, frames = 12000, team = true)
    }

    @Test
    fun `levels with an AdLib in lockstep`() {
        run(seed = 4, frames = 20000, adlib = true)
        run(seed = 5, frames = 12000, team = true, adlib = true)
    }
}
