package ugh.verify.replay

import ugh.core.game.Game

/**
 * Completeness audit of [StateProjection]: which bytes of the program image (code segments with their
 * variables, DGROUP) the port changes from frame to frame, and which of them the projection reads. A byte that
 * changes but is never read is state a reimplementation would not be checked on; each one has to be projected
 * or listed in [StateProjection.NOT_PROJECTED] with the reason.
 */
class StateAudit {
    /** Frames in which the byte at [START] + i changed. */
    val changed = IntArray(END - START)
    /** Bit mask of the phases ([PHASES]) in which it changed. */
    val phases = IntArray(END - START)
    val read = BooleanArray(END - START)
    private var previous: ByteArray? = null

    /** Wraps the read function of a projection so that every byte it reads is marked. */
    fun reading(read16: (Int, Int) -> Int): (Int, Int) -> Int = { seg, off ->
        for (a in intArrayOf(lin(seg, off), lin(seg, off + 1))) if (a in START until END) read[a - START] = true
        read16(seg, off)
    }

    /** At a boundary: marks what changed since the previous one, the stack ([stack] linear) left out. */
    fun frame(ram: ByteArray, stack: Int, phase: String) {
        val prev = previous
        if (prev == null) { previous = ram.copyOfRange(START, END); return }
        val bit = 1 shl PHASES.indexOf(phase).let { if (it < 0) PHASES.size else it }
        var i = java.util.Arrays.mismatch(prev, 0, prev.size, ram, START, END)
        while (i >= 0) {
            val a = START + i
            if (a !in stack until stack + 0x1100) { changed[i]++; phases[i] = phases[i] or bit }
            prev[i] = ram[a]
            val r = java.util.Arrays.mismatch(prev, i + 1, prev.size, ram, a + 1, END)
            i = if (r < 0) -1 else i + 1 + r
        }
    }

    fun merge(other: StateAudit) {
        for (i in changed.indices) {
            changed[i] += other.changed[i]
            phases[i] = phases[i] or other.phases[i]
            if (other.read[i]) read[i] = true
        }
    }

    /** Hidden variables that never changed (a stale entry, or not reached by the replays). */
    fun unchanged(hidden: List<StateProjection.Hidden>) =
        hidden.filter { h -> (h.linear until h.linear + h.length).none { it in START until END && changed[it - START] > 0 } }

    data class Range(val linear: Int, val length: Int, val frames: Int, val phases: List<String>) {
        val name get() = where(linear)
        override fun toString() = "%s +%d: changed in %d frames, phases %s".format(name, length, frames, phases)
    }

    /** Runs of changed, never read bytes (gaps of one byte joined, so words stay together). */
    fun unprojected(hidden: List<StateProjection.Hidden> = emptyList()): List<Range> {
        val left = BooleanArray(changed.size) { changed[it] > 0 && !read[it] }
        for (h in hidden) for (a in h.linear until h.linear + h.length) if (a in START until END) left[a - START] = false
        val bytes = changed.indices.filter { left[it] }
        val runs = ArrayList<MutableList<Int>>()
        for (i in bytes) if (runs.isNotEmpty() && i - runs.last().last() <= 2) runs.last() += i else runs += mutableListOf(i)
        return runs.map { run ->
            val mask = run.fold(0) { m, j -> m or phases[j] }
            Range(START + run.first(), run.last() - run.first() + 1, run.maxOf { changed[it] },
                (PHASES + "other").filterIndexed { b, _ -> mask and (1 shl b) != 0 })
        }
    }

    companion object {
        /** From the load segment (program code and its data segments) to the end of DGROUP. */
        const val START = 0x10000
        const val END = (Game.DGROUP shl 4) + 0x10000
        val PHASES = listOf("start", "setup", "caption", "play", "betweenLevels", "afterGame", "highScores")

        private fun lin(seg: Int, off: Int) = ((seg shl 4) + (off and 0xffff)) and 0xfffff

        /** seg:off in the segment the linear address belongs to (game code, sound driver, DGROUP). */
        fun where(a: Int): String = when {
            a in 0x16640 until 0x18780 -> "1664:%04x".format(a - 0x16640)
            a in 0x18780 until 0x1a320 -> "1878:%04x".format(a - 0x18780)
            a in 0x1a320 until 0x1a670 -> "1a32:%04x".format(a - 0x1a320)
            a >= Game.DGROUP shl 4 -> "DGROUP:%04x".format(a - (Game.DGROUP shl 4))
            a in (Game.CODE shl 4) until (Game.CODE shl 4) + 0x10000 -> "CS:%04x".format(a - (Game.CODE shl 4))
            else -> "linear %05x".format(a)
        }
    }
}
