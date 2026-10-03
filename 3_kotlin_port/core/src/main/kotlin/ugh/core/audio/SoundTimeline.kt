package ugh.core.audio

/**
 * Time line of the port's sound for a live host: emulated PIT time drives both the timer interrupts of the
 * sound code and the OPL2 samples, so register writes land at the right sample. The host forwards
 * [Host.adlib] to [write] and [Host.timerDivisor] to [divisor], calls [frame] once per game frame and
 * [untilNextTick] from [Host.timerWait]; every rendered sample goes to [sink] (at [Opl2.RATE] Hz).
 */
class SoundTimeline(private val sink: (Int) -> Unit) {
    val opl = Opl2()

    /** PIT channel 0 divisor (clocks between timer interrupts). */
    var divisor = 0x10000
        set(v) { field = if (v == 0) 0x10000 else v }

    private var toTick = 65536.0            // PIT clocks until the next timer interrupt
    private var toSample = 0.0              // PIT clocks until the next sample

    fun write(reg: Int, value: Int) = opl.write(reg, value)

    /** One game frame (1 / 70.086 s): samples, with [tick] (the timer interrupt) wherever one comes due. */
    fun frame(tick: () -> Unit) = advance(PIT_CLOCKS_PER_FRAME, tick)

    /** Time up to the next timer interrupt, which the caller then runs itself (busy wait of the game). */
    fun untilNextTick() {
        render(toTick)
        toTick = divisor.toDouble()
    }

    private fun advance(clocks: Double, tick: () -> Unit) {
        var left = clocks
        while (left >= toTick) {
            render(toTick)
            left -= toTick
            toTick = divisor.toDouble()
            tick()                           // may reprogram the divisor for the following periods
        }
        render(left)
        toTick -= left
    }

    private fun render(clocks: Double) {
        var left = clocks
        while (left >= toSample) {
            left -= toSample
            toSample = CLOCKS_PER_SAMPLE
            sink(opl.sample())
        }
        toSample -= left
    }

    companion object {
        const val PIT_HZ = 1_193_182.0
        const val FRAME_HZ = 70.086
        const val PIT_CLOCKS_PER_FRAME = PIT_HZ / FRAME_HZ
        const val CLOCKS_PER_SAMPLE = PIT_HZ / Opl2.RATE
    }
}
