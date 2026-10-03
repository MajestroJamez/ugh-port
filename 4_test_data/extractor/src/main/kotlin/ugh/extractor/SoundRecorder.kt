package ugh.extractor

import ugh.core.audio.Opl2
import ugh.core.audio.SoundTimeline
import ugh.core.game.Game
import ugh.core.game.Host
import ugh.core.game.StopGame
import ugh.core.game.channelAtStart
import ugh.core.game.timerInterrupt
import ugh.core.hw.Memory
import ugh.core.hw.MzLoader
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * The port's sound in real time, as its player hears it: the sound library of [game] runs with the timer at the
 * PIT rate it programs, its register writes go to the port's OPL2 synthesizer, and the samples are kept amplified
 * and clipped as the port's player does (`desktop/AudioOut.kt`), at [Opl2.RATE] Hz. Calls of the library go to
 * [game] (first `libInit(1)`, as the game starts), [play] lets the time pass; or the whole program runs
 * ([Game.runProgram]) with this as its host.
 */
class SoundRecorder(exe: ByteArray) : Host {
    private var pcm = ShortArray(Opl2.RATE)
    /** The samples so far. */
    var length = 0
        private set
    /** Samples beyond the 16 bits, clipped. */
    var clipped = 0
        private set
    /** Where channel [watched] went back to the start of its track: the sample of each time. */
    val restarts = mutableListOf<Int>()
    private var watched = -1
    private var atStart = true

    private val timeline = SoundTimeline { raw ->
        val v = raw * GAIN
        if (v !in Short.MIN_VALUE..Short.MAX_VALUE) clipped++
        if (length == pcm.size) pcm = pcm.copyOf(pcm.size * 2)
        pcm[length++] = v.coerceIn(Short.MIN_VALUE.toInt(), Short.MAX_VALUE.toInt()).toShort()
    }

    val game = Game(Memory().also { MzLoader.load(exe, it) }, this)

    /** The whole program may run ([Game.runProgram]): its frames end it with [StopGame] after this many samples. */
    var maxLength = Int.MAX_VALUE

    /** Lets [seconds] of game frames pass. */
    fun play(seconds: Double) = repeat((seconds * SoundTimeline.FRAME_HZ).toInt()) { step() }

    /** Lets game frames pass while [condition] holds, [maxSeconds] at most; false when the time ran out. */
    fun playWhile(maxSeconds: Double, condition: () -> Boolean): Boolean {
        repeat((maxSeconds * SoundTimeline.FRAME_HZ).toInt()) {
            if (!condition()) return true
            step()
        }
        return !condition()
    }

    /** From now on notes in [restarts] where channel [n] goes back to the start of its track (a loop repeats). */
    fun watch(n: Int) {
        watched = n
        atStart = game.channelAtStart(n)
    }

    /** The samples from [from] to [to] (exclusive). */
    fun samples(from: Int = 0, to: Int = length): ShortArray = pcm.copyOfRange(from, to)

    /** One game frame with the timer interrupts that come due in it. */
    private fun step() = timeline.frame {
        game.timerInterrupt()
        if (watched >= 0) {
            val now = game.channelAtStart(watched)
            if (now && !atStart) restarts += length
            atStart = now
        }
    }

    override fun frame() {
        if (length >= maxLength) throw StopGame()
        step()
    }

    override fun adlibPresent() = true
    override fun adlib(reg: Int, value: Int) = timeline.write(reg, value)
    override fun timerDivisor(divisor: Int) { timeline.divisor = divisor }
    override fun timerWait() = timeline.untilNextTick()

    companion object {
        /** The game drives the OPL quietly (18-33 dB down); a real AdLib made up for it in its amplifier. */
        const val GAIN = 8

        /** [samples] as a 16-bit mono WAV file at [Opl2.RATE] Hz. */
        fun writeWav(file: File, samples: ShortArray) {
            val data = ByteBuffer.allocate(samples.size * 2).order(ByteOrder.LITTLE_ENDIAN)
            samples.forEach { data.putShort(it) }
            val header = ByteBuffer.allocate(44).order(ByteOrder.LITTLE_ENDIAN).apply {
                put("RIFF".toByteArray()); putInt(36 + data.capacity()); put("WAVE".toByteArray())
                put("fmt ".toByteArray()); putInt(16); putShort(1); putShort(1); putInt(Opl2.RATE); putInt(Opl2.RATE * 2)
                putShort(2); putShort(16)
                put("data".toByteArray()); putInt(data.capacity())
            }
            file.parentFile?.mkdirs()
            file.writeBytes(header.array() + data.array())
        }
    }
}
