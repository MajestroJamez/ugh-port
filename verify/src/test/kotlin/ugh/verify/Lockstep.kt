package ugh.verify

import ugh.core.game.Game
import ugh.core.game.Host
import ugh.core.game.StopGame
import ugh.core.game.keyEvent
import ugh.core.hw.Memory
import ugh.oracle.OriginalUgh

/**
 * Lockstep differential test: the port runs from a copy of the original's state and acts as the pacemaker.
 * Every time the port waits for a retrace ([Host.frame]), the original is advanced to its next retrace
 * wait (113b:44c6) and both states are compared (RAM except the stack, whole VGA).
 *
 * Between frames: hardware interrupts of the original are held back while a frame runs and serviced at the
 * boundary; keys are delivered to both at the boundary (the original through IRQ 1, the port through its
 * keyboard handler port). State the port does not model yet (sound library, timer scheduler, BIOS data) is
 * copied from the original at every boundary.
 */
class Lockstep(private val ugh: OriginalUgh, private val maxFrames: Long, private val maxReports: Int = 5) : Host {
    val portMem = Memory()
    val game = Game(portMem, this)

    var frames = 0L
        private set
    var mismatchCount = 0
        private set
    val mismatches = mutableListOf<String>()

    /** Called at every boundary before keys are delivered; may queue keys with [key] or change both states. */
    val atBoundary = mutableListOf<(Lockstep) -> Unit>()
    private val keys = ArrayDeque<Int>()

    fun key(scancode: Int) { keys.addLast(scancode) }

    /** Files of the port (UGH!.HI); the original uses its own virtual disk, both start empty. */
    val files = mutableMapOf<String, ByteArray>()

    override fun readFile(name: String) = files[name.uppercase()]?.copyOf()

    override fun writeFile(name: String, data: ByteArray) { files[name.uppercase()] = data.copyOf() }

    /** Return address on top of the original's stack at the current retrace wait: tells which loop waits. */
    fun waitingIn(): Int = ugh.machine.read16(ugh.machine.cpu.ss, ugh.machine.cpu.sp)

    /** True once the original program has exited (after Q). */
    val originalExited get() = ugh.machine.exited

    /** The port quit: the original must get to its DOS exit without another retrace wait. */
    override fun exit() {
        val m = ugh.machine
        m.irqHold = false
        val before = ugh.gameFrame
        repeat(50) { if (!m.exited) ugh.runGameFrames(1, vgaSlack = 20) }
        if (ugh.gameFrame != before) {
            mismatchCount++
            mismatches += "the original waited %d more frames before its exit".format(ugh.gameFrame - before)
        }
        throw StopGame()
    }

    /** Writes a DGROUP word into both states (for cheats at a boundary). */
    fun poke16(off: Int, v: Int) {
        ugh.machine.write16(Game.DGROUP, off, v)
        portMem.write16(Game.DGROUP, off, v)
    }

    /** Starts the lockstep at the original's current state and runs [portCode] on the port. */
    fun run(portCode: Game.() -> Unit) {
        portMem.copyFrom(ugh.machine.memory)
        ugh.machine.irqHold = true
        try {
            game.portCode()
        } catch (_: StopGame) {
        } finally {
            ugh.machine.irqHold = false
        }
    }

    override fun frame() {
        if (frames >= maxFrames) throw StopGame()
        val m = ugh.machine
        // the original to its next retrace wait; code that busy-waits for timer ticks without a retrace wait
        // (the sound card detection at start-up) gets its held interrupts serviced
        val target = ugh.gameFrame + 1
        while (true) {
            ugh.runGameFrames(1, vgaSlack = 20)
            if (ugh.gameFrame >= target || m.exited) break
            m.irqHold = false
            m.serviceInterruptsNow()
            m.irqHold = true
        }
        if (ugh.machine.exited) throw StopGame()
        frames++
        compare()
        // boundary: interrupts, keys, not yet ported state
        m.irqHold = false
        m.serviceInterruptsNow()
        atBoundary.forEach { it(this) }
        while (keys.isNotEmpty()) {
            val sc = keys.removeFirst()
            m.deliverKeyNow(sc)
            game.keyEvent(sc)
        }
        syncNotPorted()
        m.irqHold = true
    }

    private fun syncNotPorted() {
        val a = ugh.machine.memory.ram
        val b = portMem.ram
        for ((from, to) in NOT_PORTED) System.arraycopy(a, from, b, from, to - from)
    }

    private fun compare() {
        val m = ugh.machine
        val a = m.memory.ram
        val b = portMem.ram
        val stack = m.cpu.ss shl 4
        var i = java.util.Arrays.mismatch(a, b)
        while (i >= 0 && (i in stack until stack + STACK_SIZE || NOT_PORTED.any { i in it.first until it.second })) {
            val r = java.util.Arrays.mismatch(a, i + 1, a.size, b, i + 1, b.size)
            i = if (r < 0) -1 else i + 1 + r
        }
        val problem = if (i >= 0) {
            val where = when {
                i in (Game.DGROUP shl 4) until (Game.DGROUP shl 4) + 0x10000 -> "DGROUP:%04x".format(i - (Game.DGROUP shl 4))
                i in (Game.CODE shl 4) until (Game.CODE shl 4) + 0x10000 -> "CS:%04x".format(i - (Game.CODE shl 4))
                else -> "linear %05x".format(i)
            }
            "$where original %02x port %02x".format(a[i], b[i])
        } else m.memory.vga.diff(portMem.vga)
        if (problem != null) {
            mismatchCount++
            if (mismatches.size < maxReports) {
                mismatches += "lockstep frame $frames (original at %04x:%04x, game frame %d): %s".format(
                    m.cpu.cs, m.cpu.ip, ugh.gameFrame, problem)
            }
        }
    }

    companion object {
        private const val STACK_SIZE = 0x1100

        /** Linear ranges owned by code that is not ported yet: copied from the original at every boundary. */
        val NOT_PORTED = listOf(
            0x00000 to 0x00500,                              // IVT and BIOS data area
            0x16640 to 0x18790,                              // sound driver segment 1664
            0x18780 to 0x1a670,                              // sound library 1878 and timer scheduler 1a32 code/data
            0x2a370 to 0x2a380,                              // sound library DS 2a37
            (0x6c09 shl 4) + 0x7a40 to (0x6c09 shl 4) + 0x7c10, // timer scheduler variables in DGROUP
        )
    }
}
