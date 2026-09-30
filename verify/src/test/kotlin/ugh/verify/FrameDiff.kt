package ugh.verify

import ugh.core.game.FrameResult
import ugh.core.game.Game
import ugh.core.game.Regs
import ugh.core.game.frameBody
import ugh.core.game.frameFade
import ugh.core.hw.Memory
import ugh.oracle.OriginalUgh

/**
 * Frame-level differential test: every time the original's game loop reaches 113b:0c7d, the previous frame
 * of the port is compared with the original (RAM except the stack, and the whole VGA: memory, latches,
 * registers, DAC), then the current state is copied into the port, which runs one whole frame
 * ([frameFade] + [frameBody]). Hardware interrupts are held back during the frame and serviced at its
 * boundary, before the copy, so keys arrive between frames for both.
 */
class FrameDiff(
    private val ugh: OriginalUgh,
    private val maxReports: Int = 5,
    /** Test hook: runs on the port after each frame (to check that the harness detects differences). */
    private val afterPortFrame: (Game) -> Unit = {},
) {
    var frames = 0
        private set
    var mismatchCount = 0
        private set
    val mismatches = mutableListOf<String>()
    /** Frames where the port stopped with something else than CONTINUE (not compared). */
    val special = sortedMapOf<String, Int>()

    /** Called at every frame boundary before the state is copied (state changes made here are seen by both). */
    val beforeCopy = mutableListOf<() -> Unit>()

    private val portMem = Memory()
    private val game = Game(portMem)
    private var pending = false
    private var active = true

    init {
        val m = ugh.machine
        m.onExecute(OriginalUgh.SEG_GAME, 0x0c7d) {
            if (pending) { compare(); pending = false }
            m.irqHold = false
            m.serviceInterruptsNow()
            if (!active) return@onExecute
            beforeCopy.forEach { it() }
            portMem.copyFrom(m.memory)
            val result = if (!game.frameFade()) FrameResult.LEVEL_END else game.frameBody(Regs())
            if (result == FrameResult.CONTINUE) {
                afterPortFrame(game)
                pending = true
                m.irqHold = true
            } else {
                special.merge(result.name, 1, Int::plus)
            }
        }
        // the loop is left at 0c89 (level end): do not keep interrupts held
        m.onExecute(OriginalUgh.SEG_GAME, 0x0fa7) { pending = false; m.irqHold = false }
    }

    fun stop() { active = false }

    private fun compare() {
        frames++
        val m = ugh.machine
        val stackTop = (m.cpu.ss shl 4) + m.cpu.sp
        val stackBottom = m.cpu.ss shl 4
        val a = m.memory.ram
        val b = portMem.ram
        var i = java.util.Arrays.mismatch(a, b)
        while (i in stackBottom until stackTop) {
            val r = java.util.Arrays.mismatch(a, stackTop, a.size, b, stackTop, b.size)
            i = if (r < 0) -1 else stackTop + r
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
            if (mismatches.size < maxReports) mismatches += "frame #$frames (game frame ${ugh.gameFrame}): $problem"
        }
    }
}
