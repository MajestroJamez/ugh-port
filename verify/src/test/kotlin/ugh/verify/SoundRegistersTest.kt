package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.oracle.OriginalUgh
import java.io.File
import kotlin.random.Random

/**
 * With a sound card the library returns with BX, CX, DX and ES changed in ways the port does not model.
 * Two runs of the original with an AdLib and the same input, the second with those registers overwritten by
 * random values after every return from the sound library: the game state must not diverge, i.e. the game
 * code does not depend on them.
 */
class SoundRegistersTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()

    /** Return addresses of all far calls from the game code into the sound library. */
    private val returns = intArrayOf(
        0x006a, 0x007d, 0x0184, 0x0197, 0x0289, 0x029c, 0x06d4, 0x06e7, 0x08ac, 0x08bf, 0x0c5e, 0x0c7a,
        0x0fae, 0x1aff, 0x1ce9, 0x23d3, 0x2489, 0x25ab, 0x29a7, 0x2b53, 0x4f4a, 0x4f55, 0x4f5d,
    )

    @Test
    fun `the game does not depend on registers changed by the sound library`() {
        val a = OriginalUgh(exe, adlib = true)
        val b = OriginalUgh(exe, adlib = true)
        val rnd = Random(99)
        var scrambled = 0
        val sites = sortedMapOf<Int, Int>()
        for (off in returns) b.machine.onExecute(OriginalUgh.SEG_GAME, off) {
            val c = b.machine.cpu
            c.bx = rnd.nextInt(0x10000); c.cx = rnd.nextInt(0x10000); c.dx = rnd.nextInt(0x10000)
            c.es = rnd.nextInt(0x10000)
            scrambled++
            sites.merge(off, 1, Int::plus)
        }
        for (u in listOf(a, b)) { assertTrue(u.startGame(), "game start"); CheatPilot(u, 11).attach() }
        assertEquals(a.gameFrame, b.gameFrame)
        val dgroup = OriginalUgh.SEG_DGROUP shl 4
        var compared = 0
        repeat(12_000) {
            a.runGameFrames(1); b.runGameFrames(1)
            val ra = a.machine.memory.ram; val rb = b.machine.memory.ram
            // game data in DGROUP (the timer scheduler variables from 7a40 on are sound state)
            val i = java.util.Arrays.mismatch(ra, dgroup, dgroup + 0x7a40, rb, dgroup, dgroup + 0x7a40)
            assertTrue(i < 0, "frame ${a.gameFrame}: DGROUP:%04x differs".format(i))
            assertEquals(null, a.machine.vga.diff(b.machine.vga), "frame ${a.gameFrame}: VGA")
            compared++
        }
        println("$compared frames compared, registers scrambled $scrambled times, level ${a.dgroup16(0x261c)}, sites " +
            sites.entries.joinToString(" ") { "%04x:%d".format(it.key, it.value) })
        assertTrue(sites.size >= 10, "too few call sites reached")
    }
}
