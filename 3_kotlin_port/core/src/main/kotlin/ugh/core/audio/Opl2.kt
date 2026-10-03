package ugh.core.audio

import kotlin.math.PI
import kotlin.math.ln
import kotlin.math.pow
import kotlin.math.roundToInt
import kotlin.math.sin

/**
 * Yamaha YM3812 (OPL2) synthesizer, written for this port from the documented behaviour of the chip:
 * 9 two-operator channels, sample rate = 3.579545 MHz / 72 ([RATE] Hz). Operators work in the log domain like
 * the chip (quarter log-sine table, exponent table, 9-bit envelope of 0.1875 dB steps), with the four
 * waveforms, frequency multiplier, key scale rate and level, tremolo, vibrato, feedback and both connections.
 * Not modelled because the game never uses them: rhythm mode (register BD bits 0-5), CSM, the timers' sound
 * side and the composite sine mode.
 */
class Opl2 {
    private val regs = IntArray(256)

    private class Operator {
        var mult2 = 1           // frequency multiplier * 2
        var ksr = false
        var sustained = false   // EG type
        var vib = false
        var am = false
        var totalLevel = 0      // TL in 0.75 dB
        var kslShift = 31       // KSL 0 = off
        var attack = 0
        var decay = 0
        var sustainLevel = 0    // in envelope units
        var release = 0
        var wave = 0

        var phase = 0           // 20-bit phase, one cycle = 2^20
        var level = 511         // envelope, 0 = loudest
        var state = State.RELEASE
        var out = 0
        var previousOut = 0
    }

    private enum class State { ATTACK, DECAY, SUSTAIN, RELEASE }

    private class Channel {
        var fnum = 0
        var block = 0
        var keyOn = false
        var feedback = 0
        var additive = false
    }

    private val ops = Array(18) { Operator() }
    private val channels = Array(9) { Channel() }
    private var waveSelect = false
    private var deepTremolo = false
    private var deepVibrato = false
    private var noteSelect = false
    private var egCounter = 0L
    private var amCounter = 0
    private var vibCounter = 0

    /** Operator slots of each channel's modulator; the carrier is 3 slots further. */
    private val modulatorSlot = intArrayOf(0, 1, 2, 6, 7, 8, 12, 13, 14)

    /** Register offset (0x00..0x15) -> operator slot, -1 for the unused offsets. */
    private val slotOfOffset = IntArray(0x20) { -1 }.also {
        var slot = 0
        for (group in 0 until 3) for (i in 0 until 6) it[group * 8 + i] = slot++
    }

    fun write(reg: Int, value: Int) {
        val r = reg and 0xff
        val v = value and 0xff
        regs[r] = v
        when {
            r == 0x01 -> waveSelect = v and 0x20 != 0
            r == 0x08 -> noteSelect = v and 0x40 != 0
            r == 0xbd -> { deepTremolo = v and 0x80 != 0; deepVibrato = v and 0x40 != 0 }
            r in 0x20..0x95 || r in 0xe0..0xf5 -> {
                val slot = slotOfOffset[r and 0x1f]
                if (slot < 0) return
                val op = ops[slot]
                when (r and 0xe0) {
                    0x20 -> {
                        op.am = v and 0x80 != 0; op.vib = v and 0x40 != 0
                        op.sustained = v and 0x20 != 0; op.ksr = v and 0x10 != 0
                        op.mult2 = MULT2[v and 0xf]
                    }
                    0x40 -> { op.kslShift = KSL_SHIFT[v shr 6]; op.totalLevel = v and 0x3f }
                    0x60 -> { op.attack = v shr 4; op.decay = v and 0xf }
                    0x80 -> {
                        val sl = v shr 4
                        op.sustainLevel = if (sl == 15) 0x1f0 else sl shl 4
                        op.release = v and 0xf
                    }
                    0xe0 -> op.wave = v and 3
                }
            }
            r in 0xa0..0xa8 -> channels[r - 0xa0].fnum = (channels[r - 0xa0].fnum and 0x300) or v
            r in 0xb0..0xb8 -> {
                val ch = channels[r - 0xb0]
                ch.fnum = (ch.fnum and 0xff) or ((v and 3) shl 8)
                ch.block = (v shr 2) and 7
                val on = v and 0x20 != 0
                if (on != ch.keyOn) {
                    val m = modulatorSlot[r - 0xb0]
                    for (op in listOf(ops[m], ops[m + 3])) if (on) keyOn(op) else op.state = State.RELEASE
                    ch.keyOn = on
                }
            }
            r in 0xc0..0xc8 -> {
                val ch = channels[r - 0xc0]
                ch.feedback = (v shr 1) and 7
                ch.additive = v and 1 != 0
            }
        }
    }

    private fun keyOn(op: Operator) {
        op.phase = 0
        op.state = State.ATTACK
    }

    /** Renders [count] samples at [RATE] Hz into [out] from [offset]. */
    fun generate(out: ShortArray, offset: Int, count: Int) {
        for (i in 0 until count) out[offset + i] = sample().coerceIn(-32768, 32767).toShort()
    }

    /** One output sample (sum of the 9 channels, not yet clipped). */
    fun sample(): Int {
        egCounter++
        amCounter = (amCounter + 1) % AM_PERIOD
        vibCounter = (vibCounter + 1) and 0x1fff
        // tremolo: triangle 0..26 envelope units (4.8 dB) or a quarter of it (1 dB)
        val tri = amCounter * 52 / AM_PERIOD
        val amDeep = if (tri > 26) 52 - tri else tri
        val amValue = if (deepTremolo) amDeep else amDeep shr 2
        val vibStep = VIB_PATTERN[vibCounter shr 10]

        var mix = 0
        for (c in 0 until 9) {
            val ch = channels[c]
            val mod = ops[modulatorSlot[c]]
            val car = ops[modulatorSlot[c] + 3]
            val ksrBase = (ch.block shl 1) or ((ch.fnum shr (if (noteSelect) 8 else 9)) and 1)
            val kslBase = KSL_ROM[ch.fnum shr 6] * 4 - (8 - ch.block) * 32

            val fb = if (ch.feedback != 0) (mod.previousOut + mod.out) shr (9 - ch.feedback) else 0
            val m = operator(mod, ch, ksrBase, kslBase, amValue, vibStep, fb)
            val k = operator(car, ch, ksrBase, kslBase, amValue, vibStep, if (ch.additive) 0 else m)
            mix += if (ch.additive) m + k else k
        }
        return mix
    }

    private fun operator(op: Operator, ch: Channel, ksrBase: Int, kslBase: Int, am: Int, vibStep: Int, modulation: Int): Int {
        // phase generator (vibrato moves the frequency number by up to 7/8 of its top bits)
        var fnum = ch.fnum
        if (op.vib) {
            val range = (fnum shr 7) and 7
            fnum += if (deepVibrato) range * vibStep / 2 else range * vibStep / 4
        }
        op.phase = (op.phase + (((fnum shl ch.block) * op.mult2) shr 1)) and 0xfffff
        envelope(op, ksrBase)

        var level = op.level + op.totalLevel * 4 + (maxOf(0, kslBase) shr op.kslShift)
        if (op.am) level += am
        if (level > 511) level = 511
        val p = ((op.phase shr 10) + modulation) and 0x3ff
        val value = wave(op, p, level shl 3)
        op.previousOut = op.out
        op.out = value
        return value
    }

    private fun wave(op: Operator, p: Int, envelope: Int): Int {
        val w = if (waveSelect) op.wave else 0
        var quarter = p and 0xff
        if (p and 0x100 != 0) quarter = quarter xor 0xff
        val negative = p and 0x200 != 0
        val att = when (w) {
            1 -> if (negative) return 0 else LOG_SIN[quarter]
            3 -> if (p and 0x100 != 0) return 0 else LOG_SIN[p and 0xff]
            else -> LOG_SIN[quarter]
        } + envelope
        if (att >= 0x1a00) return 0
        val v = ((EXP[(att and 0xff) xor 0xff] or 0x400) shl 1) shr (att shr 8)
        return if (negative && w == 0) -v else v
    }

    private fun envelope(op: Operator, ksrBase: Int) {
        val rate4 = when (op.state) {
            State.ATTACK -> op.attack
            State.DECAY -> op.decay
            State.SUSTAIN -> if (op.sustained) 0 else op.release
            State.RELEASE -> op.release
        }
        if (rate4 == 0) return
        val rate = minOf(63, rate4 * 4 + (if (op.ksr) ksrBase else ksrBase shr 2))
        if (op.state == State.ATTACK) {
            if (rate >= 60) op.level = 0
            else {
                val inc = increment(rate)
                if (inc > 0) op.level += (op.level.inv() * inc) shr 3
            }
            if (op.level <= 0) { op.level = 0; op.state = State.DECAY }
            return
        }
        op.level = minOf(511, op.level + increment(rate))
        if (op.state == State.DECAY && op.level >= op.sustainLevel) op.state = State.SUSTAIN
    }

    /**
     * Envelope step for [rate] at the current sample: rates below 48 step by 1 every 2^(12 - rate/4) samples
     * (on 4..7 of 8 occasions, by rate & 3), higher rates every sample by 1..8.
     */
    private fun increment(rate: Int): Int {
        val hi = rate shr 2
        val lo = rate and 3
        return if (hi <= 12) {
            val shift = 12 - hi
            if (egCounter and ((1L shl shift) - 1) != 0L) 0
            else STEP_PATTERN[lo][((egCounter shr shift) and 7).toInt()]
        } else if (hi < 15) {
            val base = 1 shl (hi - 13)
            base * (1 + STEP_PATTERN[lo][(egCounter and 7).toInt()])
        } else 4
    }

    companion object {
        /** Native sample rate: 3.579545 MHz / 72. */
        const val RATE = 49716

        private val MULT2 = intArrayOf(1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30)
        private val KSL_SHIFT = intArrayOf(31, 1, 2, 0)   // 0, 3, 1.5, 6 dB/octave
        private val KSL_ROM = intArrayOf(0, 32, 40, 45, 48, 51, 53, 55, 56, 58, 59, 60, 61, 62, 63, 64)
        private val VIB_PATTERN = intArrayOf(0, 1, 2, 1, 0, -1, -2, -1)
        private val STEP_PATTERN = arrayOf(
            intArrayOf(0, 1, 0, 1, 0, 1, 0, 1),
            intArrayOf(0, 1, 0, 1, 1, 1, 0, 1),
            intArrayOf(0, 1, 1, 1, 0, 1, 1, 1),
            intArrayOf(0, 1, 1, 1, 1, 1, 1, 1),
        )
        private const val AM_PERIOD = 13440                // 3.7 Hz

        /** -log2(sin) of the first quarter wave, in 1/256 units. */
        private val LOG_SIN = IntArray(256) { (-ln(sin((it + 0.5) * PI / 512)) / ln(2.0) * 256).roundToInt() }

        /** 2^x - 1 for the fraction x = i/256, in 1/1024 units. */
        private val EXP = IntArray(256) { ((2.0.pow(it / 256.0) - 1) * 1024).roundToInt() }
    }
}
