package ugh.verify

import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.audio.Opl2
import ugh.core.audio.SoundTimeline
import ugh.core.game.Game
import ugh.core.game.Host
import ugh.core.game.libControl
import ugh.core.game.libEffect
import ugh.core.game.libInit
import ugh.core.game.libMusic
import ugh.core.game.libStop
import ugh.core.game.timerInterrupt
import ugh.core.hw.Memory
import ugh.core.hw.MzLoader
import java.io.ByteArrayOutputStream
import java.io.File
import kotlin.math.abs
import kotlin.math.sqrt

/**
 * The port's sound in real time: the library runs with the timer at the PIT rate it programs, the register
 * writes go to the OPL2 synthesizer. Renders the three pieces of music, every effect and a game-like mix to
 * WAV files in build/verify-out/sound (for listening; they contain the game's music, never commit them).
 */
class SoundRenderTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val out = File(System.getProperty("ugh.out"), "sound").also { it.mkdirs() }

    private companion object {
        /** The game drives the OPL quietly (18-33 dB down); a real AdLib made up for it in its amplifier. */
        const val GAIN = 8
    }

    private class Renderer : Host {
        val pcm = ByteArrayOutputStream()
        var samples = 0
        var clipped = 0
        var sumSquares = 0.0
        lateinit var game: Game
        private val timeline = SoundTimeline { raw ->
            val v = raw * GAIN
            if (abs(v) > 32767) clipped++
            val s = v.coerceIn(-32768, 32767)
            sumSquares += s.toDouble() * s
            pcm.write(s and 0xff); pcm.write((s shr 8) and 0xff)
            samples++
        }

        override fun frame() = error("no frames")
        override fun adlibPresent() = true
        override fun adlib(reg: Int, value: Int) = timeline.write(reg, value)
        override fun timerDivisor(divisor: Int) { timeline.divisor = divisor }
        override fun timerWait() = timeline.untilNextTick()

        /** Lets [seconds] of game frames pass, the timer interrupts at the programmed rate. */
        fun play(seconds: Double) = repeat((seconds * SoundTimeline.FRAME_HZ).toInt()) { timeline.frame { game.timerInterrupt() } }

        val rms get() = sqrt(sumSquares / maxOf(1, samples))
    }

    private fun session(body: Renderer.(Game) -> Unit): Renderer {
        val mem = Memory()
        MzLoader.load(exe, mem)
        val r = Renderer()
        val game = Game(mem, r)
        r.game = game
        game.libInit(1)
        r.body(game)
        return r
    }

    private fun save(name: String, r: Renderer) {
        val data = r.pcm.toByteArray()
        val header = java.nio.ByteBuffer.allocate(44).order(java.nio.ByteOrder.LITTLE_ENDIAN).apply {
            put("RIFF".toByteArray()); putInt(36 + data.size); put("WAVE".toByteArray())
            put("fmt ".toByteArray()); putInt(16); putShort(1); putShort(1); putInt(Opl2.RATE); putInt(Opl2.RATE * 2)
            putShort(2); putShort(16)
            put("data".toByteArray()); putInt(data.size)
        }
        File(out, "$name.wav").writeBytes(header.array() + data)
        println("%s: %.1f s, rms %.0f, %d clipped".format(name, r.samples.toDouble() / Opl2.RATE, r.rms, r.clipped))
        assertTrue(r.rms > 800, "$name is silent")
        assertTrue(r.clipped < r.samples / 1000, "$name clips")
    }

    @Test
    fun `music and effects render to sound`() {
        for ((name, seg) in listOf("music-menu" to 0x4637, "music-game" to 0x4444, "music-scores" to 0x4279)) {
            save(name, session { g -> g.libMusic(0, seg, 0, 0xa5); play(45.0); g.libControl(0x11); play(0.5) })
        }
        val effects = intArrayOf(0x425b, 0x4260, 0x4265, 0x426b, 0x4274, 0x4629, 0x4632)
        save("effects", session { g ->
            for (seg in effects) {
                val ch = g.libEffect(0, seg, 0xff, if (seg == 0x4260) 0 else 1)
                play(2.5)
                g.libStop(ch)
                play(0.3)
            }
        })
        // the level music with effects on top, as in the game
        save("game-mix", session { g ->
            g.libMusic(0, 0x4444, 0, 0xa5)
            play(3.0)
            val flap = g.libEffect(0, 0x4260, 0xff, 0)
            for (i in 0 until 12) {
                play(0.7)
                g.libEffect(0, effects[i % effects.size], 0xff, 1)
                if (i == 8) g.libStop(flap)
            }
            play(3.0)
            g.libControl(0x11)
            play(0.5)
        })
    }
}
