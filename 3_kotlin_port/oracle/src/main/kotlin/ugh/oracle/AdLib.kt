package ugh.oracle

/**
 * AdLib (YM3812 / OPL2) as seen from the CPU: address port 0x388 (read = status), data port 0x389.
 * Only what a program can observe is modelled - the register file and the two timers with their status
 * flags (used by every AdLib detection routine). Every register write is recorded with its emulated time,
 * which is what the sound port is compared against and what a synthesizer plays.
 */
class AdLib(private val ips: Long) {
    val regs = IntArray(256)
    private var index = 0

    /** A register write: [time] in emulated instructions. */
    data class Write(val time: Long, val reg: Int, val value: Int)

    /** Register writes since the last [drain] (only while [recording]). */
    val writes = ArrayList<Write>()
    var recording = false

    /** Called for every register write (e.g. a synthesizer of the desktop window). */
    var listener: ((Write) -> Unit)? = null

    fun drain(): List<Write> { val w = ArrayList(writes); writes.clear(); return w }

    // timers: T1 counts in 80 us steps, T2 in 320 us steps, from the preset up to 256
    private var t1Start = -1L
    private var t2Start = -1L
    private var t1Flag = false
    private var t2Flag = false

    private fun period(preset: Int, us: Long) = (256 - preset) * us * ips / 1_000_000L

    private fun update(time: Long) {
        val ctl = regs[4]
        if (t1Start >= 0 && time - t1Start >= period(regs[2], 80)) {
            if (ctl and 0x40 == 0) t1Flag = true
            t1Start += period(regs[2], 80).coerceAtLeast(1) * ((time - t1Start) / period(regs[2], 80).coerceAtLeast(1))
        }
        if (t2Start >= 0 && time - t2Start >= period(regs[3], 320)) {
            if (ctl and 0x20 == 0) t2Flag = true
            t2Start += period(regs[3], 320).coerceAtLeast(1) * ((time - t2Start) / period(regs[3], 320).coerceAtLeast(1))
        }
    }

    fun status(time: Long): Int {
        update(time)
        var s = 0
        if (t1Flag) s = s or 0xc0
        if (t2Flag) s = s or 0xa0
        return s or 0x06 // the low bits of a real OPL2 read as 6
    }

    fun writeIndex(v: Int) { index = v and 0xff }

    fun writeData(v: Int, time: Long) {
        update(time)
        val value = v and 0xff
        if (index == 4) {
            if (value and 0x80 != 0) {
                t1Flag = false; t2Flag = false          // IRQ reset, the other bits are ignored
            } else {
                regs[4] = value
                t1Start = if (value and 1 != 0) (if (t1Start >= 0) t1Start else time) else -1
                t2Start = if (value and 2 != 0) (if (t2Start >= 0) t2Start else time) else -1
                if (value and 0x40 != 0) t1Flag = false
                if (value and 0x20 != 0) t2Flag = false
            }
        } else {
            regs[index] = value
        }
        val w = Write(time, index, value)
        if (recording) writes += w
        listener?.invoke(w)
    }
}
