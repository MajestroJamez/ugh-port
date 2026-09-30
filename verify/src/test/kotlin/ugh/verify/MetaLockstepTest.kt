package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.runProgram
import ugh.extractor.Exe
import ugh.extractor.Levels
import ugh.oracle.OriginalUgh
import java.io.File

/**
 * The whole program in lockstep, from right after the C runtime start-up (113b:000d) to Q in the main menu:
 * intro, attract sequence, menu options, password entry, a game until game over, the "bad luck" screen,
 * a high score entry with name input and UGH!.HI, and quitting.
 */
class MetaLockstepTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val levels = Levels.extract(Exe(exe))

    /** Waiting loops of the original, identified by the return address of the retrace wait. */
    private object At {
        const val INTRO_OR_PAGE = 0x44b2      // 4497: picture / credits page / high scores / bad luck
        const val CUT_SCENE = 0x0911
        const val MENU = 0x00de
        const val CONTROLS = 0x0603
        const val PASSWORD = 0x04e7
        const val NAME = 0x02dd
        const val CAPTION = 0x44e1            // 44db: wait for a key
        const val GAME = 0x0ca5
    }

    private class Director(private val ls: Lockstep) {
        private val steps = ArrayDeque<Pair<Int, () -> Unit>>()
        private val pendingBreaks = mutableListOf<Pair<Int, Int>>() // frames left, break code
        var gameFrames = 0
        val log = mutableListOf<String>()
        private var last = -1

        fun step(at: Int, action: () -> Unit) { steps.addLast(at to action) }

        fun tap(make: Int) { ls.key(make); pendingBreaks += 3 to (make or 0x80) }

        fun type(text: String) {
            // one key per boundary via a small queue of taps
            for (ch in text) typing.addLast(ch)
        }
        val typing = ArrayDeque<Char>()
        private var typeDelay = 0

        fun onBoundary() {
            val it = pendingBreaks.listIterator()
            while (it.hasNext()) { val (n, code) = it.next(); if (n <= 1) { ls.key(code); it.remove() } else it.set(n - 1 to code) }
            val where = ls.waitingIn()
            val arrived = where != last
            if (arrived) { log += "%d:%04x".format(ls.frames, where); last = where }
            if (where == At.GAME) gameFrames++
            if (typing.isNotEmpty()) {
                if (--typeDelay <= 0) { tap(scancode(typing.removeFirst())); typeDelay = 4 }
                return
            }
            // the level caption comes again after every lost life: always confirmed with space
            if (arrived && where == At.CAPTION && steps.firstOrNull()?.first != At.CAPTION) { tap(0x39); return }
            val next = steps.firstOrNull() ?: return
            if (next.first == where && pendingBreaks.isEmpty()) { steps.removeFirst(); next.second() }
        }

        private fun scancode(c: Char): Int = when (c) {
            in 'A'..'Z' -> LETTERS[c - 'A']
            in '1'..'9' -> 0x02 + (c - '1')
            '0' -> 0x0b
            ' ' -> 0x39
            '\n' -> 0x1c
            '\b' -> 0x0e
            else -> error("cannot type $c")
        }

        companion object {
            private val LETTERS = intArrayOf(
                0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
                0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c,
            )
        }
    }

    @Test
    fun `the whole program matches the original`() {
        val ugh = OriginalUgh(exe)
        assertTrue(ugh.runUntil(0x113b, 0x000d, 100), "start of main not reached")
        val ls = Lockstep(ugh, maxFrames = 60_000)
        val d = Director(ls)
        val pilot = Pilot(ugh, seed = 21)
        var scored = false
        ls.atBoundary += {
            d.onBoundary()
            pilot.enabled = ls.waitingIn() == At.GAME
            if (pilot.enabled && !scored) {
                scored = true
                ls.poke16(0x261e, 5000)                  // enough for the high score table
            }
        }
        pilot.enabled = false
        pilot.attachTo(ls)
        val password = levels.passwordsOnePlayer[2]
        with(d) {
            step(At.INTRO_OR_PAGE) { tap(0x01) }             // Esc: skip the rest of the intro
            step(At.CUT_SCENE) { tap(0x2d) }                 // any key: next attract screen
            step(At.INTRO_OR_PAGE) { tap(0x2d) }             // credits
            step(At.CUT_SCENE) { tap(0x2d) }                 // second scene
            step(At.INTRO_OR_PAGE) { tap(0x2d) }             // high scores -> menu
            step(At.MENU) { tap(0x3d) }                      // F3 difficulty
            step(At.MENU) { tap(0x3e) }                      // F4 team
            step(At.MENU) { tap(0x3e) }                      // F4 one player
            step(At.MENU) { tap(0x3f) }                      // F5 controls
            step(At.CONTROLS) { tap(0x3b) }                  // F1: player 1 device
            step(At.CONTROLS) { tap(0x01) }                  // Esc
            step(At.MENU) { tap(0x3c) }                      // F2 password
            step(At.PASSWORD) { type(password + "\n") }
            step(At.MENU) { tap(0x3b) }                      // F1 play
            step(At.CUT_SCENE) { tap(0x2d) }                 // skip the opening scene
            step(At.CAPTION) { tap(0x39) }                   // level caption
            step(At.NAME) { type("\n") }                     // empty high score name: the default one
            step(At.INTRO_OR_PAGE) { tap(0x2d) }             // high score table
            step(At.MENU) { tap(0x10) }                      // Q: quit
        }
        ls.run { runProgram() }
        println("whole program: ${ls.frames} frames, ${ls.mismatchCount} mismatches, ${d.gameFrames} game frames, " +
            "original exited ${ls.originalExited}, UGH!.HI written ${ls.files.keys}")
        println("waits: " + d.log.joinToString(" ").take(3000))
        ls.mismatches.forEach(::println)
        assertEquals(0, ls.mismatchCount)
        assertTrue(ls.originalExited, "the original did not reach its exit")
        assertTrue(ls.files.containsKey("UGH!.HI"), "high score file not written")
        assertEquals(ugh.machine.disk.files["UGH!.HI"]?.toList(), ls.files["UGH!.HI"]?.toList(), "UGH!.HI contents")
    }

    @Test
    fun `timeouts, team mode, the ending and an existing high score file match the original`() {
        val ugh = OriginalUgh(exe)
        assertTrue(ugh.runUntil(0x113b, 0x000d, 100), "start of main not reached")
        // a high score file from an earlier session (the default table)
        val hi = ByteArray(0x6b) { (ugh.dgroup8(0xc86 + it) xor 0xff).toByte() }
        val ls = Lockstep(ugh, maxFrames = 60_000)
        ugh.machine.disk.files["UGH!.HI"] = hi.copyOf()
        ls.files["UGH!.HI"] = hi.copyOf()
        val d = Director(ls)
        val pilot = CheatPilot(ugh, seed = 5)
        var played = 0
        ls.atBoundary += {
            d.onBoundary()
            pilot.enabled = ls.waitingIn() == At.GAME
            if (pilot.enabled && ++played == 300) {
                // the last level ends as completed (as 113b:149c does after the last passenger)
                ls.poke16(0x261e, 20000)                 // enough for the high score table
                ls.poke16(0x27aa, 0xfffe)
                ls.poke16(0x27a8, (ugh.dgroup16(0x27a8) - 2) and 0xffff)
                ls.poke16(0x27ce, (ugh.dgroup16(0x27ce) and 0xff) or 0xfe00)
            }
        }
        pilot.enabled = false
        pilot.attachTo(ls)
        val password = levels.passwordsTeam.last()
        with(d) {
            // no keys: the intro and the attract sequence run out, the menu times out into another attract round
            step(At.MENU) { }
            step(At.CUT_SCENE) { }
            step(At.MENU) { tap(0x3e) }                      // F4 team
            step(At.MENU) { tap(0x3f) }                      // F5 controls
            step(At.CONTROLS) { tap(0x3c) }                  // F2: player 2 device
            step(At.CONTROLS) { tap(0x2d) }                  // another key: redraw
            step(At.CONTROLS) { tap(0x01) }                  // Esc
            step(At.MENU) { tap(0x2d) }                      // unknown key
            step(At.MENU) { tap(0x3c) }                      // F2 password, typed with a correction
            step(At.PASSWORD) { type("X1 \b\b\b$password\n") }
            step(At.MENU) { tap(0x3b) }                      // F1 play
            step(At.NAME) { type("\bTEAM ONF\bE\n") }
            step(At.INTRO_OR_PAGE) { tap(0x2d) }
            step(At.MENU) { tap(0x10) }                      // Q
        }
        ls.run { runProgram() }
        println("ending: ${ls.frames} frames, ${ls.mismatchCount} mismatches, ${d.gameFrames} game frames, " +
            "level ${ugh.dgroup16(0x261c)}, original exited ${ls.originalExited}")
        println("waits: " + d.log.joinToString(" ").take(3000))
        ls.mismatches.forEach(::println)
        assertEquals(0, ls.mismatchCount)
        assertTrue(ls.originalExited, "the original did not reach its exit")
        assertEquals(ugh.machine.disk.files["UGH!.HI"]?.toList(), ls.files["UGH!.HI"]?.toList(), "UGH!.HI contents")
    }

    @Test
    fun `Q in the intro, in the attract sequence and Esc in a cut scene match the original`() {
        for ((name, script) in listOf<Pair<String, Director.() -> Unit>>(
            "Q in the intro" to { step(At.INTRO_OR_PAGE) { tap(0x10) } },
            "Q in the title scene" to { step(At.INTRO_OR_PAGE) { tap(0x01) }; step(At.CUT_SCENE) { tap(0x10) } },
            "Esc in the title scene, Q in the credits" to {
                step(At.INTRO_OR_PAGE) { tap(0x01) }; step(At.CUT_SCENE) { tap(0x01) }; step(At.INTRO_OR_PAGE) { tap(0x10) }
            },
        )) {
            val ugh = OriginalUgh(exe)
            assertTrue(ugh.runUntil(0x113b, 0x000d, 100), "start of main not reached")
            val ls = Lockstep(ugh, maxFrames = 20_000)
            val d = Director(ls)
            ls.atBoundary += { d.onBoundary() }
            d.script()
            ls.run { runProgram() }
            println("$name: ${ls.frames} frames, ${ls.mismatchCount} mismatches, original exited ${ls.originalExited}, " +
                "waits ${d.log.joinToString(" ")}")
            ls.mismatches.forEach(::println)
            assertEquals(0, ls.mismatchCount, name)
            assertTrue(ls.originalExited, "$name: the original did not reach its exit")
        }
    }
}
