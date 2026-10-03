package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.Regs
import ugh.core.game.bonusesUpdate
import ugh.core.game.passengersUpdate
import ugh.oracle.OriginalUgh
import java.io.File

class PassengersTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()

    private fun regs(r: CallDiff.Regs) = Regs(r.ax, r.bx, r.cx, r.dx, r.si, r.di, r.bp)

    private fun check(team: Boolean, seed: Long, frames: Long, cheat: Boolean = false) {
        val ugh = OriginalUgh(exe)
        assertTrue(ugh.startGame(if (team) listOf(0x3e) else emptyList()))
        val passengers = CallDiff(ugh, "passengersUpdate", 0x1486, { g, r -> g.passengersUpdate(regs(r)) })
        val bonuses = CallDiff(ugh, "bonusesUpdate", 0x2b7f, { g, r -> g.bonusesUpdate(regs(r)) })
        val cheatPilot = if (cheat) CheatPilot(ugh, seed).also { it.attach() } else null
        if (!cheat) Pilot(ugh, seed, player = 0).attach()
        if (team) Pilot(ugh, seed + 100, player = 1).attach()
        // which passenger states were visited during play
        val states = sortedSetOf<Int>()
        passengers.onCall { g ->
            var bx = 0
            while (g.u(0x29ad + bx) != 0xffff && bx < 0x20) { states += g.u(0x2a0d + bx); bx += 2 }
        }
        val bonusStates = sortedSetOf<Int>()
        bonuses.onCall { g -> for (bx in 0..0x16 step 2) if (g.u(0x2d9b + bx) != 0xffff) bonusStates += g.u(0x2d83 + bx) }
        val levels = sortedSetOf<Int>()
        passengers.onCall { g -> levels += g.u(0x261c) }
        ugh.runGameFrames(frames)
        val mode = (if (team) "team" else "1P") + (if (cheat) " cheat" else "") +
            " levels ${levels.map { it + 1 }} bonus states ${bonusStates.joinToString(" ") { "%04x".format(it) }}"
        println("$mode seed $seed: passengers ${passengers.calls} calls / ${passengers.mismatchCount} mismatches, " +
            "bonuses ${bonuses.calls} / ${bonuses.mismatchCount}; states seen: ${states.joinToString(" ") { "%04x".format(it) }}" +
            (cheatPilot?.let { "; actions ${it.actions}" } ?: ""))
        (passengers.mismatches + bonuses.mismatches).forEach(::println)
        assertTrue(passengers.calls > 500)
        assertEquals(0, passengers.mismatchCount)
        assertEquals(0, bonuses.mismatchCount)
    }

    @Test
    fun `passengers and bonuses match the original, one player`() {
        for (seed in 1L..3L) check(team = false, seed = seed, frames = 4000)
    }

    @Test
    fun `passengers and bonuses match the original, pickups and deliveries`() {
        for (seed in 1L..4L) check(team = false, seed = seed, frames = 30000, cheat = true)
    }

    @Test
    fun `passengers and bonuses match the original, team mode`() {
        for (seed in 1L..2L) check(team = true, seed = seed, frames = 4000)
    }
}
