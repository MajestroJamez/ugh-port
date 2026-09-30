package ugh.verify

import ugh.core.game.Game
import ugh.core.hw.Memory
import ugh.oracle.OriginalUgh

/**
 * Checks one ported routine against the original: at every call of [entry] in the running oracle the whole
 * state (RAM + VGA) is copied into a fresh port instance and [port] is run on it; when the original returns,
 * both states are compared byte for byte (the stack below the caller's SP excluded).
 * Hardware interrupts are held back while the original routine runs, so only the routine itself is compared.
 */
class CallDiff(
    private val ugh: OriginalUgh,
    private val name: String,
    private val entry: Int,
    private val port: (game: Game, regs: Regs) -> Unit,
    private val maxReports: Int = 5,
) {
    data class Regs(val ax: Int, val bx: Int, val cx: Int, val dx: Int, val si: Int, val di: Int, val bp: Int)

    var calls = 0
        private set
    val mismatches = mutableListOf<String>()
    var mismatchCount = 0
        private set

    private val portMem = Memory()
    private val game = Game(portMem)
    private var pendingReturnSp = -1
    private var pendingRegs: Regs? = null

    init {
        val m = ugh.machine
        m.onExecute(OriginalUgh.SEG_GAME, entry) {
            if (pendingReturnSp < 0) {
                val c = m.cpu
                val regs = Regs(c.ax, c.bx, c.cx, c.dx, c.si, c.di, c.bp)
                portMem.copyFrom(m.memory)
                port(game, regs)
                pendingRegs = regs
                pendingReturnSp = c.sp + 2
                val ret = m.read16(c.ss, c.sp)
                watchReturn(ret)
                m.irqHold = true
            }
        }
    }

    private val watchedReturns = HashSet<Int>()

    private fun watchReturn(ret: Int) {
        if (!watchedReturns.add(ret)) return
        val m = ugh.machine
        m.onExecute(OriginalUgh.SEG_GAME, ret) {
            if (pendingReturnSp >= 0 && m.cpu.sp == pendingReturnSp) {
                m.irqHold = false
                compare(pendingRegs!!)
                pendingReturnSp = -1
            }
        }
    }

    private fun compare(regs: Regs) {
        calls++
        val m = ugh.machine
        val stackTop = (m.cpu.ss shl 4) + m.cpu.sp   // bytes below the caller's SP are scratch
        val stackBottom = m.cpu.ss shl 4
        val a = m.memory.ram
        val b = portMem.ram
        var i = java.util.Arrays.mismatch(a, b)
        while (i in stackBottom until stackTop) i = mismatchFrom(a, b, stackTop)
        val problem = if (i >= 0) {
            val seg = when {
                i in (Game.DGROUP shl 4) until (Game.DGROUP shl 4) + 0x10000 -> "DGROUP:%04x".format(i - (Game.DGROUP shl 4))
                i in (Game.CODE shl 4) until (Game.CODE shl 4) + 0x10000 -> "CS:%04x".format(i - (Game.CODE shl 4))
                else -> "linear %05x".format(i)
            }
            "$seg original %02x port %02x".format(a[i], b[i])
        } else m.memory.vga.diff(portMem.vga)
        if (problem != null) mismatchCount++
        if (problem != null && mismatches.size < maxReports) {
            mismatches += "$name call #$calls (game frame ${ugh.gameFrame}, $regs): $problem"
        }
    }

    private fun mismatchFrom(a: ByteArray, b: ByteArray, from: Int): Int {
        val r = java.util.Arrays.mismatch(a, from, a.size, b, from, b.size)
        return if (r < 0) -1 else from + r
    }
}
