package ugh.extractor

import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.audio.Opl2
import ugh.core.game.libEffect
import ugh.core.game.libInit
import ugh.core.game.libMusic
import java.io.File
import kotlin.math.abs
import kotlin.math.sqrt

/**
 * The sounds for the remake, measured (nobody listened to them here): every one has a length that fits its kind, is
 * heard, is not clipped; a loop repeats like the port's: a pass is as long as the next one, as loud at every moment,
 * and it goes on into its own start without a click. That the files are what the port's player plays: verify's
 * SoundRenderTest.
 */
class SoundsTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()

    @Test
    fun `every sound is heard, not clipped, of a length that fits`() {
        for (sound in Sounds.ALL) {
            val stats = Sounds.stats(Sounds.render(exe, sound))
            println("${sound.name}: $stats")
            val seconds = when (sound.kind) {
                Sounds.Kind.EFFECT, Sounds.Kind.LOOP -> 0.2..2.0
                Sounds.Kind.MUSIC -> 60.0..120.0
            }
            assertTrue(stats.seconds in seconds, "${sound.name}: ${stats.seconds} s")
            assertTrue(stats.rms > 200, "${sound.name} is silent")
            assertTrue(stats.peak < Short.MAX_VALUE, "${sound.name} clips")
        }
    }

    @Test
    fun `a loop repeats its pass without a seam`() {
        for (sound in Sounds.ALL.filter { it.kind != Sounds.Kind.EFFECT }) {
            val r = SoundRecorder(exe)
            r.game.libInit(1)
            val channel = if (sound.kind == Sounds.Kind.MUSIC) {
                r.game.libMusic(0, sound.segment, 0, sound.volume)
                Sounds.MUSIC_CHANNEL
            } else {
                r.game.libEffect(0, sound.segment, sound.volume, 0)
            }
            r.watch(channel)
            r.playWhile(400.0) { r.restarts.size < 3 }
            val (a, b, c) = r.restarts
            val pass = r.samples(a, b)
            assertTrue(pass.contentEquals(Sounds.render(exe, sound)), "${sound.name}: the file is not the second pass")
            assertTrue(abs((c - b) - (b - a)) <= 1, "${sound.name}: passes of ${b - a} and ${c - b} samples")
            val loudness = similarity(envelope(pass), envelope(r.samples(b, b + pass.size)))
            val seam = abs(pass.last() - pass.first())
            val steepest = (1 until pass.size).maxOf { abs(pass[it] - pass[it - 1]) }
            println("${sound.name}: a pass of ${pass.size} samples, its loudness like the next one's: %.4f, seam %d (steepest %d)"
                .format(loudness, seam, steepest))
            assertTrue(loudness > 0.99, "${sound.name}: the passes differ (%.4f)".format(loudness))
            assertTrue(seam <= steepest, "${sound.name}: a click where it repeats ($seam)")
        }
    }

    /** The loudness (RMS) of every 10 ms of [samples]. */
    private fun envelope(samples: ShortArray): DoubleArray {
        val window = Opl2.RATE / 100
        return DoubleArray(samples.size / window) { w ->
            sqrt((0 until window).sumOf { val s = samples[w * window + it].toDouble(); s * s } / window)
        }
    }

    private fun similarity(a: DoubleArray, b: DoubleArray): Double {
        var ab = 0.0; var aa = 0.0; var bb = 0.0
        for (i in a.indices) { ab += a[i] * b[i]; aa += a[i] * a[i]; bb += b[i] * b[i] }
        return ab / sqrt(aa * bb)
    }
}
