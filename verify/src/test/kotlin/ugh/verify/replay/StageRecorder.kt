package ugh.verify.replay

import ugh.oracle.OriginalUgh

/**
 * What the later stages of a play frame change outside their own entities, for checking a reimplementation
 * that does not have all of them yet: snapshots of the original's projection when the passengers (113b:1486),
 * the objects (2363) and the bonus items (2b7f) start and when the bonus items return. Per stage, the fields
 * outside its group (passengers: everything but passenger.*, ...) that it changed, with their values before
 * it ("~" = the field did not exist). A reimplementation without the stage undoes these on the expected state.
 */
class StageRecorder(private val ugh: OriginalUgh) {
    private val projection = StateProjection { seg, off -> ugh.machine.read16(seg, off) }
    private val snapshots = ArrayList<Pair<Int, Map<String, String>>>()
    private val returns = HashSet<Int>()

    fun attach() {
        val m = ugh.machine
        fun snap(point: Int) { snapshots += point to projection.project("play") }
        m.onExecute(OriginalUgh.SEG_GAME, 0x1486) { snap(0) }
        m.onExecute(OriginalUgh.SEG_GAME, 0x2363) { snap(1) }
        m.onExecute(OriginalUgh.SEG_GAME, 0x2b7f) {
            snap(2)
            // near call: the return address is on top of the stack
            val ret = m.read16(m.cpu.ss, m.cpu.sp)
            if (returns.add(ret)) m.onExecute(OriginalUgh.SEG_GAME, ret) { snap(3) }
        }
    }

    /** At a boundary: the stage changes of the frame that just ended (empty unless it ran the level play logic). */
    fun frameDone(): Map<String, Map<String, String>> {
        val s = snapshots.takeLast(4)
        snapshots.clear()
        if (s.map { it.first } != listOf(0, 1, 2, 3)) return emptyMap()
        val result = linkedMapOf<String, Map<String, String>>()
        for ((i, entry) in STAGES.withIndex()) {
            val (stage, own) = entry
            val before = s[i].second
            val after = s[i + 1].second
            val changed = (before.keys + after.keys).filter { !it.startsWith(own) && before[it] != after[it] }
            if (changed.isNotEmpty()) result[stage] = changed.sorted().associateWith { before[it] ?: "~" }
        }
        return result
    }

    companion object {
        /** Stage names and the field group each stage owns. */
        val STAGES = listOf("passengers" to "passenger.", "objects" to "object.", "bonuses" to "bonus.")
    }
}
