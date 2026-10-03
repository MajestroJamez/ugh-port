package ugh.verify

import org.junit.jupiter.api.Assumptions.assumeTrue
import org.junit.jupiter.api.Test
import ugh.core.game.runProgram
import ugh.oracle.OriginalUgh
import java.io.File

/**
 * Exploration run with an AdLib: which instructions of the sound code (1664, 1878, 1a32) execute during
 * the intro, the menu and some levels, and how many register writes happen. Writes the executed addresses
 * to build/verify-out/sound-exec.txt (input for the disassembly passes). Run with -Pugh.explore.
 */
class SoundExploreTest {
    @Test
    fun `executed sound code`() {
        assumeTrue(System.getProperty("ugh.explore") != null, "exploration only")
        val ugh = OriginalUgh(File(System.getProperty("ugh.exe")).readBytes(), adlib = true)
        val cov = BooleanArray(0x100000)
        ugh.machine.coverage = cov
        val adlib = ugh.machine.adlib!!
        adlib.recording = true
        ugh.runGameFrames(600)                              // intro
        println("intro: ${adlib.writes.size} AdLib writes, sound handle table ${ugh.dgroup16(0x2d61)}")
        check(ugh.startGame(log = ::println)) { "game did not start" }
        CheatPilot(ugh, 7).attach()
        ugh.runGameFrames(8000)
        println("after play: ${adlib.writes.size} AdLib writes, 2d61 = %04x".format(ugh.dgroup16(0x2d61)))
        val regsUsed = adlib.writes.groupingBy { it.reg }.eachCount().toSortedMap()
        println("registers: " + regsUsed.entries.joinToString(" ") { "%02x:%d".format(it.key, it.value) })
        val out = File(System.getProperty("ugh.out")).also { it.mkdirs() }
        val lines = ArrayList<String>()
        for ((seg, len) in listOf(0x1664 to 0x2140, 0x1878 to 0x1bb8, 0x1a32 to 0x350)) {
            for (off in 0 until len) if (cov[(seg shl 4) + off]) lines += "%04x:%04x".format(seg, off)
        }
        File(out, "sound-exec.txt").writeText(lines.joinToString("\n"))
        File(out, "adlib-writes.txt").writeText(adlib.writes.take(20000).joinToString("\n") {
            "%d %02x %02x".format(it.time, it.reg, it.value)
        })
        println("${lines.size} executed sound instructions")
    }
}

/** Which instructions of the ported sound routines the AdLib lockstep scenarios leave unexecuted. */
class SoundCoverageTest {
    @Test
    fun `sound code covered by the lockstep scenarios`() {
        assumeTrue(System.getProperty("ugh.explore") != null, "exploration only")
        val exe = File(System.getProperty("ugh.exe")).readBytes()
        val cov = BooleanArray(0x100000)
        fun lockstep(ugh: OriginalUgh, frames: Long, body: (Lockstep) -> Unit) {
            ugh.machine.coverage = cov
            val ls = Lockstep(ugh, frames)
            body(ls)
            check(ls.mismatchCount == 0) { ls.mismatches.joinToString("\n") }
        }
        // whole program from the start, menu keys, a game with the cheat pilot
        val a = OriginalUgh(exe, adlib = true)
        a.runUntil(0x113b, 0x000d, 100)
        lockstep(a, 30_000) { ls ->
            val pilot = CheatPilot(a, 21).also { it.attachTo(ls) }
            var t = 0
            ls.atBoundary += {
                pilot.enabled = ls.waitingIn() == 0x0ca5
                if (ls.waitingIn() == 0x00de && ++t % 200 == 0) { ls.key(0x3b); }
                if (ls.waitingIn() == 0x00de && t % 200 == 3) ls.key(0xbb)
            }
            ls.run { runProgram() }
        }
        SoundCoverage.report(cov)
    }
}

/** Prints the instructions of the ported sound routines that a run left unexecuted (needs re/out/listing.asm). */
object SoundCoverage {
    fun report(cov: BooleanArray) {
        val lst = File("../re/out/listing.asm").readLines()
        val ins = lst.mapNotNull { Regex("^(1664|1878|1a32):([0-9a-f]{4}) ").find(it) }
            .map { it.groupValues[1].toInt(16) * 16 + it.groupValues[2].toInt(16) }.toSortedSet()
        val ported = listOf(
            0x1664 to (0x1211 to 0x1b39), 0x1878 to (0x0003 to 0x00f4), 0x1878 to (0x0691 to 0x06e3),
            0x1878 to (0x0a70 to 0x0f2c), 0x1878 to (0x0f88 to 0x11ac), 0x1878 to (0x14bb to 0x1942),
            0x1a32 to (0x0008 to 0x0340),
        )
        for ((seg, range) in ported) {
            var runStart = -1
            val missing = ArrayList<String>()
            for (off in range.first..range.second + 1) {
                val lin = seg * 16 + off
                val miss = off <= range.second && lin in ins && !cov[lin]
                if (miss && runStart < 0) runStart = off
                if (!miss && runStart >= 0 && (off > range.second || lin in ins)) {
                    missing += "%04x-%04x".format(runStart, off - 1); runStart = -1
                }
            }
            println("%04x %04x..%04x not executed: %s".format(seg, range.first, range.second, missing.joinToString(" ")))
        }
    }
}
