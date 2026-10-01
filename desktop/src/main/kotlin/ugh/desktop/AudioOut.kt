package ugh.desktop

import ugh.core.audio.Opl2
import javax.sound.sampled.AudioFormat
import javax.sound.sampled.AudioSystem
import javax.sound.sampled.SourceDataLine

/**
 * Sound card output: takes the synthesizer's samples ([Opl2.RATE] Hz), resamples them to 48 kHz and writes
 * them to a Java Sound line. The game thread is paced by the 70 Hz frame clock, the line by the sound card;
 * small differences between the two clocks are absorbed by dropping or repeating a few samples when the
 * line's buffer gets too full or too empty. Without a sound device everything is discarded.
 */
class AudioOut(private val gain: Int = 8) : AutoCloseable {
    private val format = AudioFormat(OUT_RATE.toFloat(), 16, 1, true, false)
    private val line: SourceDataLine? = try {
        AudioSystem.getSourceDataLine(format).apply { open(format, BUFFER_BYTES); start() }
    } catch (e: Exception) {
        System.err.println("no sound output: ${e.message}")
        null
    }

    private val pending = ByteArray(OUT_RATE)          // one second of output at most per batch
    private var count = 0
    private var position = 0.0                         // resampler: position between previous and current input
    private var previous = 0
    private var last = 0

    /** One synthesizer sample. */
    fun sample(raw: Int) {
        val current = (raw * gain).coerceIn(-32768, 32767)
        // linear interpolation from the OPL rate down to 48 kHz
        while (position <= 1.0) {
            val v = (previous + (current - previous) * position).toInt()
            if (count < pending.size / 2) { pending[2 * count] = v.toByte(); pending[2 * count + 1] = (v shr 8).toByte(); count++ }
            last = v
            position += STEP
        }
        position -= 1.0
        previous = current
    }

    /** End of a game frame: queue what it produced without blocking, keeping the latency near its target. */
    fun endFrame() {
        val l = line ?: run { count = 0; return }
        val queued = (l.bufferSize - l.available()) / 2
        var n = count
        if (queued > TARGET * 2 && n > 0) n -= minOf(n, 1 + n / 50)               // too much queued: drop
        if (queued < TARGET / 2) repeat(minOf(16, pending.size / 2 - n)) { put(n++, last) } // nearly empty: stretch
        val bytes = minOf(n * 2, l.available())
        l.write(pending, 0, bytes)
        count = 0
    }

    /** Busy wait of the game (music fade-out): write everything, blocking, so it takes real time. */
    fun flushBlocking() {
        val l = line ?: run { count = 0; return }
        l.write(pending, 0, count * 2)
        count = 0
    }

    private fun put(i: Int, v: Int) { pending[2 * i] = v.toByte(); pending[2 * i + 1] = (v shr 8).toByte() }

    override fun close() {
        line?.run { stop(); flush(); close() }
    }

    private companion object {
        const val OUT_RATE = 48_000
        const val STEP = Opl2.RATE.toDouble() / OUT_RATE
        const val TARGET = OUT_RATE * 60 / 1000          // 60 ms queued
        const val BUFFER_BYTES = OUT_RATE * 2 * 200 / 1000  // 200 ms line buffer
    }
}
