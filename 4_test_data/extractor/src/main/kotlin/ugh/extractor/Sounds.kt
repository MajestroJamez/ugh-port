package ugh.extractor

import ugh.core.audio.Opl2
import ugh.core.game.channelPlaying
import ugh.core.game.libEffect
import ugh.core.game.libInit
import ugh.core.game.libMusic
import java.io.File
import kotlin.math.abs
import kotlin.math.log10
import kotlin.math.sqrt
import kotlin.system.exitProcess

/**
 * The sounds of the original for the remake (5_remake/game): every ADLX block played by the port's sound library the
 * way the game plays it (its volume, once or as a loop) and rendered by the port's OPL2 synthesizer into a WAV file of
 * assets/sound. Where the port's game plays each: the comments of [ALL] (`core/game`).
 */
object Sounds {
    /** How the game plays a block: an effect once, an effect repeated until stopped, or the music (it repeats). */
    enum class Kind { EFFECT, LOOP, MUSIC }

    /** The file [name].wav of the ADLX block in segment [segment], played at [volume] (0 .. 0xff). */
    data class Sound(val name: String, val segment: Int, val kind: Kind, val volume: Int)

    val ALL = listOf(
        Sound("caption", 0x4629, Kind.EFFECT, 0xe1),     // the caption of a level (Level.kt)
        Sound("game-over", 0x426b, Kind.EFFECT, 0xe1),   // "bad luck" after a lost game (Meta.kt, afterGame)
        Sound("screech", 0x425b, Kind.EFFECT, 0xff),     // the flyer screeches before it flies (Objects.kt)
        Sound("flap", 0x4260, Kind.LOOP, 0xff),          // the flyer flaps across the screen (Objects.kt)
        Sound("blow", 0x4265, Kind.EFFECT, 0xff),        // the blower blows (Objects.kt)
        Sound("bonus", 0x4274, Kind.EFFECT, 0xff),       // a bonus item drops: out of a tree, a quick delivery
        Sound("drop", 0x4632, Kind.EFFECT, 0xff),        // a pilot lets the hanging passenger go (Passengers.kt)
        Sound("music-menu", 0x4637, Kind.MUSIC, 0xa5),   // the main menu (Meta.kt, mainMenu)
        Sound("music-game", 0x4444, Kind.MUSIC, 0xa5),   // the play of a level (GameFlow.kt)
        Sound("music-ending", 0x4279, Kind.MUSIC, 0xa5), // the ending after the last level and the high scores
    )

    /** The music's channel of the library (the effects get 8 .. 11). */
    const val MUSIC_CHANNEL = 12
    /** The longest an effect or a pass of a loop may take. */
    private const val MAX_SECONDS = 300.0
    /** After an effect: time for the release of its last notes. */
    private const val TAIL_SECONDS = 0.5

    /**
     * The samples of [sound]: an effect from its first sound to its last; a loop one pass of its track, the second
     * one (the first starts from silence, the second with the end of the first fading out, as every later pass), so
     * the file repeats without a seam.
     */
    fun render(exe: ByteArray, sound: Sound): ShortArray {
        val r = SoundRecorder(exe)
        r.game.libInit(1)
        if (sound.kind == Kind.EFFECT) {
            val channel = r.game.libEffect(0, sound.segment, sound.volume, 1)
            check(r.playWhile(MAX_SECONDS) { r.game.channelPlaying(channel) }) { "${sound.name} does not end" }
            r.play(TAIL_SECONDS)
            return trimSilence(r.samples())
        }
        val channel = if (sound.kind == Kind.MUSIC) {
            r.game.libMusic(0, sound.segment, 0, sound.volume)
            MUSIC_CHANNEL
        } else {
            r.game.libEffect(0, sound.segment, sound.volume, 0)
        }
        r.watch(channel)
        check(r.playWhile(2 * MAX_SECONDS) { r.restarts.size < 2 }) { "${sound.name} does not repeat" }
        return r.samples(r.restarts[0], r.restarts[1])
    }

    /** [samples] without the silence before and after. */
    private fun trimSilence(samples: ShortArray): ShortArray {
        val first = samples.indexOfFirst { it.toInt() != 0 }
        if (first < 0) return ShortArray(0)
        return samples.copyOfRange(first, samples.indexOfLast { it.toInt() != 0 } + 1)
    }

    /** Length, loudness and peak of [samples]. */
    data class Stats(val seconds: Double, val rms: Double, val peak: Int) {
        val peakDbfs get() = 20 * log10(maxOf(peak, 1) / 32768.0)
        override fun toString() = "%.2f s, rms %.0f, peak %d (%.1f dBFS)".format(seconds, rms, peak, peakDbfs)
    }

    fun stats(samples: ShortArray): Stats {
        var sum = 0.0
        var peak = 0
        for (s in samples) { sum += s.toDouble() * s; peak = maxOf(peak, abs(s.toInt())) }
        return Stats(samples.size.toDouble() / Opl2.RATE, sqrt(sum / maxOf(1, samples.size)), peak)
    }

    /** Normalized correlation of two signals of the same length: 1 when they are the same, 0 when unrelated. */
    fun similarity(a: ShortArray, b: ShortArray): Double {
        require(a.size == b.size) { "signals of different lengths" }
        var ab = 0.0; var aa = 0.0; var bb = 0.0
        for (i in a.indices) { ab += a[i].toDouble() * b[i]; aa += a[i].toDouble() * a[i]; bb += b[i].toDouble() * b[i] }
        return ab / sqrt(aa * bb)
    }
}

/** usage: Sounds <UGH.EXE> <output dir> - writes <output dir>/<name>.wav for every sound of [Sounds.ALL]. */
fun main(args: Array<String>) {
    if (args.size != 2) {
        System.err.println("usage: Sounds <UGH.EXE> <output dir>")
        exitProcess(2)
    }
    val exe = readKnownExe(File(args[0])).bytes
    val out = File(args[1])
    for (sound in Sounds.ALL) {
        val samples = Sounds.render(exe, sound)
        SoundRecorder.writeWav(File(out, "${sound.name}.wav"), samples)
        println("${sound.name}.wav: ${Sounds.stats(samples)}")
    }
    println("Rendered ${Sounds.ALL.size} sounds into ${out.absolutePath}")
}
