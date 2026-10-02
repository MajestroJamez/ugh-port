package ugh.verify

import ugh.core.game.Game
import ugh.core.game.Host
import ugh.core.game.StopGame
import ugh.core.game.keyEvent
import ugh.core.game.timerInterrupt
import ugh.core.hw.Memory
import ugh.core.hw.MzLoader
import ugh.oracle.OriginalUgh

/**
 * Lockstep differential test: the port runs from a copy of the original's state and acts as the pacemaker.
 * Every time the port waits for a retrace ([Host.frame]), the original is advanced to its next retrace
 * wait (113b:44c6) and both states are compared (RAM except the stack, whole VGA).
 *
 * Between frames: hardware interrupts of the original are held back while a frame runs and serviced at the
 * boundary; keys are delivered to both at the boundary (the original through IRQ 1, the port through its
 * keyboard handler port); timer interrupts the original takes there run the port's timer interrupt too, and the
 * AdLib register writes of every frame are compared as well. State the port does not model (BIOS data, the
 * CPU speed measured by the sound driver) is
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

    /** Called with every scancode delivered at a boundary (after [atBoundary]), e.g. to record a replay. */
    val onKey = mutableListOf<(Int) -> Unit>()

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
        val before = ugh.gameFrame
        repeat(50) { if (!m.exited) runOriginal() }
        if (ugh.gameFrame != before) {
            mismatchCount++
            mismatches += "the original waited %d more frames before its exit".format(ugh.gameFrame - before)
        }
        compareAdlib()
        throw StopGame()
    }

    // ------------------------------------------------------------ sound

    private val adlib = ugh.machine.adlib
    private val portWrites = ArrayList<Pair<Int, Int>>()

    /** Number of AdLib register writes compared so far. */
    var adlibWrites = 0L
        private set

    /** Timer interrupts the port ran at frame boundaries (mirroring the original's IRQ 0). */
    var timerInterrupts = 0L
        private set

    override fun adlibPresent() = adlib != null

    private var originalWaitTicks = 0L
    private var portWaitTicks = 0L
    private var retryTicks = 0L

    override fun timerWait() { portWaitTicks++ }

    override fun adlib(reg: Int, value: Int) { portWrites += reg to value }

    init {
        adlib?.recording = true
        // 1878:10f3: the library waits for the fade-out in a loop without retrace wait; each pass of the
        // original gets one timer interrupt, like each pass of the port's loop (Game.waitTimer)
        ugh.machine.onExecute(0x1878, 0x10f3) {
            if (ugh.machine.read16(0x1664, 0x258) != 0) {
                val before = ugh.machine.pic.count[0]
                ugh.machine.timerInterruptNow()
                originalWaitTicks += ugh.machine.pic.count[0] - before
            }
        }
    }

    private fun compareAdlib() {
        val a = adlib
        if (a == null) { portWrites.clear(); return }
        val original = a.drain().map { it.reg to it.value }
        if (original != portWrites) {
            val i = original.indices.firstOrNull { it >= portWrites.size || original[it] != portWrites[it] } ?: portWrites.size
            report("AdLib write #%d of the frame: original %s port %s (%d / %d writes)".format(
                i, original.getOrNull(i)?.hex(), portWrites.getOrNull(i)?.hex(), original.size, portWrites.size))
        }
        adlibWrites += original.size
        portWrites.clear()
    }

    private fun Pair<Int, Int>.hex() = "%02x=%02x".format(first, second)

    private fun report(problem: String) {
        mismatchCount++
        if (mismatches.size < maxReports) {
            mismatches += "lockstep frame $frames (original at %04x:%04x, game frame %d): %s".format(
                ugh.machine.cpu.cs, ugh.machine.cpu.ip, ugh.gameFrame, problem)
        }
    }

    /**
     * Runs the original to its next retrace wait. Code that busy-waits for the timer without a retrace wait
     * (the CPU speed calibration at start-up) gets its held interrupts serviced; the port cannot mirror a
     * sound timer interrupt there, so one would be reported.
     */
    private fun runOriginal() {
        val m = ugh.machine
        val target = ugh.gameFrame + 1
        while (true) {
            ugh.runGameFrames(1, vgaSlack = 20)
            if (ugh.gameFrame >= target || m.exited) break
            val ticks = m.pic.count[0]
            m.irqHold = false
            m.serviceInterruptsNow()
            m.irqHold = true
            retryTicks += m.pic.count[0] - ticks
            if (m.pic.count[0] != ticks && soundTimerInstalled()) report("timer interrupt inside a frame (not mirrored)")
        }
    }

    private fun soundTimerInstalled() = ugh.machine.read16(0, 0x22) == 0x1a32 && ugh.machine.read16(0, 0x20) == 0x00d2

    /** Writes a DGROUP word into both states (for cheats at a boundary). */
    fun poke16(off: Int, v: Int) {
        ugh.machine.write16(Game.DGROUP, off, v)
        portMem.write16(Game.DGROUP, off, v)
    }

    /** Starts the lockstep at the original's current state and runs [portCode] on the port. */
    fun run(freshExe: ByteArray? = null, portCode: Game.() -> Unit) {
        if (freshExe == null) portMem.copyFrom(ugh.machine.memory)
        else {
            // the port from the bare program image, without what the C runtime start-up wrote
            MzLoader.load(freshExe, portMem)
            ignored += RUNTIME_STARTUP
            syncNotPorted()
        }
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
        val frameStart = m.pic.count[0]
        runOriginal()
        if (ugh.machine.exited) throw StopGame()
        frames++
        if (m.pic.count[0] - frameStart - retryTicks != originalWaitTicks || originalWaitTicks != portWaitTicks) {
            report("timer interrupts in the frame: original %d (%d in its wait loop), port %d".format(
                m.pic.count[0] - frameStart, originalWaitTicks, portWaitTicks))
        }
        originalWaitTicks = 0; portWaitTicks = 0; retryTicks = 0
        compare()
        // boundary: interrupts, keys, not yet ported state
        m.irqHold = false
        val ticks = m.pic.count[0]
        m.serviceInterruptsNow()
        syncNotPorted()                      // the port's interrupt goes where the original's INT 8 vector points
        repeat((m.pic.count[0] - ticks).toInt()) { game.timerInterrupt(); timerInterrupts++ }
        atBoundary.forEach { it(this) }
        while (keys.isNotEmpty()) {
            val sc = keys.removeFirst()
            onKey.forEach { it(sc) }
            // a timer tick that came due while the boundary interrupts ran is serviced first (higher priority)
            val before = m.pic.count[0]
            m.deliverKeyNow(sc)
            repeat((m.pic.count[0] - before).toInt()) { game.timerInterrupt(); timerInterrupts++ }
            game.keyEvent(sc)
        }
        syncNotPorted()
        m.irqHold = true
    }

    /** Further linear ranges left out of the comparison. */
    val ignored = ArrayList<Pair<Int, Int>>()

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
        while (i >= 0 && (i in stack until stack + STACK_SIZE || NOT_PORTED.any { i in it.first until it.second } ||
                ignored.any { i in it.first until it.second })) {
            val r = java.util.Arrays.mismatch(a, i + 1, a.size, b, i + 1, b.size)
            i = if (r < 0) -1 else i + 1 + r
        }
        val problem = if (i >= 0) {
            val where = when {
                i in 0x16640 until 0x18780 -> "1664:%04x".format(i - 0x16640)
                i in 0x18780 until 0x1a320 -> "1878:%04x".format(i - 0x18780)
                i in 0x1a320 until 0x1a670 -> "1a32:%04x".format(i - 0x1a320)
                i in (Game.DGROUP shl 4) until (Game.DGROUP shl 4) + 0x10000 -> "DGROUP:%04x".format(i - (Game.DGROUP shl 4))
                i in (Game.CODE shl 4) until (Game.CODE shl 4) + 0x10000 -> "CS:%04x".format(i - (Game.CODE shl 4))
                else -> "linear %05x".format(i)
            }
            "$where original %02x port %02x".format(a[i], b[i])
        } else m.memory.vga.diff(portMem.vga)
        if (problem != null) report(problem)
        compareAdlib()
    }

    companion object {
        private const val STACK_SIZE = 0x1100

        /** Linear ranges the port does not model: copied from the original at every boundary. */
        val NOT_PORTED = listOf(
            0x00000 to 0x00500,                              // IVT and BIOS data area
            0x16640 + 0x1127 to 0x16640 + 0x112b,            // CPU speed measured by the sound driver
        )

        /** Written by the Borland C runtime before main: environment, PSP, its variables, streams, heap. */
        val RUNTIME_STARTUP = listOf(
            0x0fe00 to 0x10000, 0x10250 to 0x10258, 0x10790 to 0x107a0,
            0x6c0e0 to 0x6c130, 0x73c80 to 0x74ee0,
            0xf0000 to 0x100000,                             // the emulator's BIOS ROM
        )
    }
}
