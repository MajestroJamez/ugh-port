package ugh.verify

import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.core.audio.Opl2
import ugh.core.game.StopGame
import ugh.core.game.libControl
import ugh.core.game.libEffect
import ugh.core.game.libInit
import ugh.core.game.libMusic
import ugh.core.game.libStop
import ugh.core.game.runProgram
import ugh.extractor.SoundRecorder
import ugh.extractor.Sounds
import java.io.File
import kotlin.math.abs

/**
 * The port's sound in real time (SoundRecorder): renders the three pieces of music, every effect and a game-like mix
 * to WAV files in build/verify-out/sound (for listening; they contain the game's music, never commit them), and
 * checks the remake's sounds (the extractor's Sounds, `:extractor:sound`) against what the port's player plays.
 */
class SoundRenderTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val out = File(System.getProperty("ugh.out"), "sound").also { it.mkdirs() }

    private fun session(body: SoundRecorder.() -> Unit) = SoundRecorder(exe).apply { game.libInit(1); body() }

    private fun save(name: String, r: SoundRecorder) {
        val samples = r.samples()
        SoundRecorder.writeWav(File(out, "$name.wav"), samples)
        val stats = Sounds.stats(samples)
        println("$name: $stats, ${r.clipped} clipped")
        assertTrue(stats.rms > 800, "$name is silent")
        assertTrue(r.clipped < r.length / 1000, "$name clips")
    }

    @Test
    fun `music and effects render to sound`() {
        for ((name, seg) in listOf("music-menu" to 0x4637, "music-game" to 0x4444, "music-scores" to 0x4279)) {
            save(name, session { game.libMusic(0, seg, 0, 0xa5); play(45.0); game.libControl(0x11); play(0.5) })
        }
        val effects = intArrayOf(0x425b, 0x4260, 0x4265, 0x426b, 0x4274, 0x4629, 0x4632)
        save("effects", session {
            for (seg in effects) {
                val ch = game.libEffect(0, seg, 0xff, if (seg == 0x4260) 0 else 1)
                play(2.5)
                game.libStop(ch)
                play(0.3)
            }
        })
        // the level music with effects on top, as in the game
        save("game-mix", session {
            game.libMusic(0, 0x4444, 0, 0xa5)
            play(3.0)
            val flap = game.libEffect(0, 0x4260, 0xff, 0)
            for (i in 0 until 12) {
                play(0.7)
                game.libEffect(0, effects[i % effects.size], 0xff, 1)
                if (i == 8) game.libStop(flap)
            }
            play(3.0)
            game.libControl(0x11)
            play(0.5)
        })
    }

    /**
     * The whole program without keys - the intro, then the main menu with its music - as the port's player plays it:
     * a pass of its menu music is the remake's file, as close as two passes of the port's music are to each other
     * (not the same: the synthesizer's tremolo and vibrato run on through a repeat).
     */
    @Test
    fun `the remake's menu music is what the port plays in its menu`() {
        val music = Sounds.render(exe, Sounds.ALL.first { it.name == "music-menu" })
        val port = SoundRecorder(exe)
        port.watch(Sounds.MUSIC_CHANNEL)
        port.maxLength = 3 * music.size + 40 * Opl2.RATE
        try {
            port.game.runProgram()
        } catch (_: StopGame) {
        }
        val starts = port.restarts
        assertTrue(starts.size >= 2 && abs(starts[1] - starts[0] - music.size) <= 1 &&
            starts[1] + music.size <= port.length, "no two passes of ${music.size} samples in the port's menu ($starts)")
        val first = port.samples(starts[0], starts[0] + music.size)
        val passes = Sounds.similarity(first, port.samples(starts[1], starts[1] + music.size))
        val similarity = Sounds.similarity(music, first)
        println("menu music: %d samples, like the port's pass at %d: %.4f, the port's two passes: %.4f"
            .format(music.size, starts[0], similarity, passes))
        assertTrue(similarity > 0.99 && similarity >= passes - 0.001, "the menu music differs from the port's")
    }
}
