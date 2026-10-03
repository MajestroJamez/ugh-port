package ugh.verify

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.game.Game
import ugh.core.game.Host
import ugh.core.game.libControl
import ugh.core.game.libEffect
import ugh.core.game.libMusic
import ugh.core.game.libStop
import ugh.core.game.timerInterrupt
import ugh.core.hw.Memory
import ugh.oracle.OriginalUgh
import java.io.File
import kotlin.random.Random

/**
 * The sound library and driver call by call: random sequences of the library calls the game makes (effects
 * of all ADLX blocks with odd volumes and repeat counts, stops of any channel, music, fade-outs) and runs of
 * timer interrupts, each done by the original (a far call into it / its INT 8) and by the port from the same
 * state. Memory, return values and AdLib writes must be equal after every step. Reaches what the game rarely
 * does: four effects at once and stealing a channel, effects taking the music's voices, voices moving when an
 * effect ends, tracks ending and repeating.
 */
class SoundLibraryTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()

    private val blocks = intArrayOf(0x425b, 0x4260, 0x4265, 0x426b, 0x4274, 0x4279, 0x4444, 0x4629, 0x4632, 0x4637)
    private val music = intArrayOf(0x4279, 0x4444, 0x4637)

    private class PortHost(val present: Boolean) : Host {
        val writes = ArrayList<Pair<Int, Int>>()
        var waits = 0
        override fun frame() = error("no frames in library calls")
        override fun adlibPresent() = present
        override fun adlib(reg: Int, value: Int) { writes += reg to value }
        override fun timerWait() { waits++ }
    }

    @Test
    fun `random library calls and timer interrupts match the original`() {
        val ugh = OriginalUgh(exe, adlib = true)
        assertTrue(ugh.runUntil(0x113b, 0x00de, 20_000), "main menu")
        val m = ugh.machine
        val coverage = if (System.getProperty("ugh.explore") != null) BooleanArray(0x100000) else null
        m.coverage = coverage
        val adlib = m.adlib!!
        adlib.recording = true
        m.irqHold = true
        // return address of the far calls: a NOP at F000:FF00 where the run stops
        m.write8(0xf000, 0xff00, 0x90)
        m.onExecute(0xf000, 0xff00) { m.stop() }
        var originalWaits = 0
        m.onExecute(0x1878, 0x10f3) {
            if (m.read16(0x1664, 0x258) != 0) {
                val before = m.pic.count[0]
                m.timerInterruptNow()
                originalWaits += (m.pic.count[0] - before).toInt()
            }
        }
        val portMem = Memory().also { it.copyFrom(m.memory) }
        val host = PortHost(present = true)
        val game = Game(portMem, host)
        adlib.drain()

        val rnd = Random(2024)
        val stack = (m.cpu.ss shl 4) + m.cpu.sp
        var steps = 0
        var writes = 0L
        val ops = sortedMapOf<String, Int>()

        fun farCall(seg: Int, off: Int, vararg args: Int): Int {
            val c = m.cpu
            val saved = intArrayOf(c.ax, c.bx, c.cx, c.dx, c.si, c.di, c.bp, c.sp, c.ds, c.es, c.cs, c.ip, c.flags)
            for (a in args.reversed()) c.push(a and 0xffff)
            c.push(0xf000); c.push(0xff00)
            c.cs = seg; c.ip = off
            m.run(200_000_000L)
            check(c.cs == 0xf000 && c.ip == 0xff00) { "far call %04x:%04x did not return".format(seg, off) }
            val ax = c.ax
            c.ax = saved[0]; c.bx = saved[1]; c.cx = saved[2]; c.dx = saved[3]; c.si = saved[4]; c.di = saved[5]
            c.bp = saved[6]; c.sp = saved[7]; c.ds = saved[8]; c.es = saved[9]; c.cs = saved[10]; c.ip = saved[11]
            c.flags = saved[12]
            return ax
        }

        fun check(what: String, originalAx: Int?, portAx: Int?) {
            steps++
            val a = m.memory.ram; val b = portMem.ram
            var i = java.util.Arrays.mismatch(a, b)
            // the stack below the frame of the waiting game, IVT/BIOS data and the measured CPU speed are not compared
            while (i >= 0 && (i < 0x500 || i in stack - 0x400 until stack || i in 0x16640 + 0x1127 until 0x16640 + 0x112b)) {
                val r = java.util.Arrays.mismatch(a, i + 1, a.size, b, i + 1, b.size)
                i = if (r < 0) -1 else i + 1 + r
            }
            val where = when {
                i in 0x16640 until 0x18780 -> "1664:%04x".format(i - 0x16640)
                i in 0x6c090 until 0x7c090 -> "DGROUP:%04x".format(i - 0x6c090)
                else -> "linear %05x".format(i)
            }
            if (i >= 0) org.junit.jupiter.api.Assertions.fail<Unit>("step $steps ($what): $where original %02x port %02x".format(a[i], b[i]))
            if (originalAx != null) assertEquals(originalAx, portAx, "step $steps ($what): AX")
            val ow = adlib.drain().map { it.reg to it.value }
            assertEquals(ow, host.writes, "step $steps ($what): AdLib writes")
            assertEquals(originalWaits, host.waits, "step $steps ($what): timer interrupts while waiting")
            writes += ow.size
            host.writes.clear()
        }

        repeat(6000) {
            val op = rnd.nextInt(100)
            when {
                op < 30 -> {
                    val seg = blocks[rnd.nextInt(blocks.size)]
                    val vol = if (rnd.nextInt(8) == 0) rnd.nextInt(0x300) else rnd.nextInt(0x100)
                    val rep = rnd.nextInt(4)
                    val o = farCall(0x1878, 0x1852, 0, seg, vol, rep)
                    val p = game.libEffect(0, seg, vol, rep)
                    ops.merge("effect", 1, Int::plus)
                    check("effect %04x vol %x x%d".format(seg, vol, rep), o, p)
                }
                op < 40 -> {
                    val n = 7 + rnd.nextInt(7)
                    farCall(0x1878, 0x1768, n); game.libStop(n)
                    ops.merge("stop", 1, Int::plus)
                    check("stop $n", null, null)
                }
                op < 46 -> {
                    val seg = if (rnd.nextBoolean()) music[rnd.nextInt(music.size)] else blocks[rnd.nextInt(blocks.size)]
                    val fade = if (rnd.nextInt(3) == 0) 1 + rnd.nextInt(3) else 0
                    val vol = rnd.nextInt(0x100)
                    val o = farCall(0x1878, 0x0f88, 0, seg, fade, vol)
                    val p = game.libMusic(0, seg, fade, vol)
                    ops.merge("music", 1, Int::plus)
                    check("music %04x fade %d vol %x".format(seg, fade, vol), o, p)
                }
                op < 50 -> {
                    val fade = if (rnd.nextBoolean()) 0 else 1 + rnd.nextInt(3)
                    val o = farCall(0x1878, 0x1059, fade)
                    val p = game.libControl(fade)
                    ops.merge("control", 1, Int::plus)
                    check("control $fade", o, p)
                }
                else -> {
                    repeat(1 + rnd.nextInt(if (rnd.nextInt(10) == 0) 2000 else 40)) {
                        val before = m.pic.count[0]
                        m.timerInterruptNow()
                        // a long tick can let the next interrupt in (nested after the BIOS clock's EOI)
                        repeat((m.pic.count[0] - before).toInt()) { game.timerInterrupt() }
                    }
                    ops.merge("ticks", 1, Int::plus)
                    check("timer interrupts", null, null)
                }
            }
        }
        println("$steps steps $ops, $writes AdLib writes compared")
        coverage?.let(SoundCoverage::report)
    }
}
