package ugh.core.audio

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import kotlin.math.abs
import kotlin.math.log10

class Opl2Test {
    /** Channel 0 as a plain sine on the carrier: modulator silent, carrier full level, sustained. */
    private fun sine(opl: Opl2, fnum: Int, block: Int, attack: Int = 15, release: Int = 15) {
        opl.write(0x20, 0x01); opl.write(0x40, 0x3f); opl.write(0x60, 0xf0); opl.write(0x80, 0x0f)
        opl.write(0x23, 0x21); opl.write(0x43, 0x00); opl.write(0x63, (attack shl 4) or 0); opl.write(0x83, release)
        opl.write(0xc0, 0x00)
        opl.write(0xa0, fnum and 0xff); opl.write(0xb0, 0x20 or (block shl 2) or (fnum shr 8))
    }

    private fun render(opl: Opl2, samples: Int) = ShortArray(samples).also { opl.generate(it, 0, samples) }

    private fun zeroCrossingFrequency(s: ShortArray): Double {
        var crossings = 0
        for (i in 1 until s.size) if (s[i - 1] < 0 && s[i] >= 0) crossings++
        return crossings.toDouble() * Opl2.RATE / s.size
    }

    @Test
    fun `a note sounds at the frequency of its F-number and block`() {
        // f = fnum * 49716 / 2^(20 - block): fnum 580, block 4 -> 440.0 Hz
        val opl = Opl2()
        sine(opl, 580, 4)
        val s = render(opl, Opl2.RATE)
        assertEquals(440.0, zeroCrossingFrequency(s), 1.0)
        val peak = s.maxOf { abs(it.toInt()) }
        assertTrue(peak in 4000..4100, "full level sine peak $peak")
    }

    @Test
    fun `the multiplier and total level scale frequency and amplitude`() {
        val opl = Opl2()
        sine(opl, 580, 4)
        opl.write(0x23, 0x22)                       // multiplier 2
        opl.write(0x43, 0x08)                       // 6 dB down
        val s = render(opl, Opl2.RATE)
        assertEquals(880.0, zeroCrossingFrequency(s), 2.0)
        val peak = s.maxOf { abs(it.toInt()) }
        assertEquals(-6.0, 20 * log10(peak / 4084.0), 0.3)
    }

    @Test
    fun `key off releases the note to silence, a slow attack rises gradually`() {
        val opl = Opl2()
        sine(opl, 580, 4, attack = 15, release = 8)
        render(opl, 2000)
        opl.write(0xb0, (4 shl 2) or (580 shr 8))   // key off
        val tail = render(opl, Opl2.RATE)
        assertTrue(tail.takeLast(1000).all { it.toInt() == 0 }, "silent after the release")
        assertTrue(tail.take(100).any { abs(it.toInt()) > 1000 }, "release is not instant")

        val slow = Opl2()
        sine(slow, 580, 4, attack = 6)
        val rise = render(slow, Opl2.RATE / 2)
        val early = rise.take(500).maxOf { abs(it.toInt()) }
        val late = rise.takeLast(500).maxOf { abs(it.toInt()) }
        assertTrue(early < late / 2, "attack rises: $early -> $late")
    }

    @Test
    fun `frequency modulation adds overtones and feedback works`() {
        val opl = Opl2()
        sine(opl, 580, 4)
        opl.write(0x40, 0x10)                       // modulator audible
        opl.write(0xc0, 0x0e)                       // feedback 7
        val s = render(opl, Opl2.RATE)
        // a modulated wave crosses zero more often than the plain 440 Hz sine
        assertTrue(zeroCrossingFrequency(s) > 450, "modulation changes the waveform")
    }
}
