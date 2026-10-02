package ugh.verify.replay

import java.io.File
import java.io.Writer

/**
 * Golden replay file, format "UGR 0" (plain ASCII text, one record per line, easy to parse from C++):
 *
 *   UGR 0
 *   # free comment lines
 *   meta <key>=<value> ...             recording conditions (exe hash, start level, seed, mode, ...)
 *   T <tick> w=<wait> k=<keys> | <field>=<value> ...
 *   B <stage> <field>=<value> ...      what a stage of the preceding T line's frame changed (see below)
 *   I <field>=<value> ...              state injection after the preceding T line (see below)
 *
 * One T line per frame (70.086 Hz tick of the original). The fields are the state *after* that frame
 * ([StateProjection]); only fields that changed since the previous T line are written, a field that
 * disappeared (e.g. a bonus item slot became free) is written as <field>=~. The first T line holds the
 * complete state. k= lists the scancodes (hex, comma separated, "-" for none) delivered *after* the frame,
 * i.e. the input of the next frame. w= is the return address of the retrace wait the original stopped at
 * (which loop of the game the frame belongs to: level play, caption, fade, ...).
 *
 * B lines (after the T line of a frame that ran the level play logic, also the last one before the next level
 * starts; in stage order passengers, objects, bonuses): the fields outside
 * the stage's own group (passengers: all but passenger.*, objects: all but object.*, bonuses: all but bonus.*)
 * that the stage changed during the frame, with their values right before it ("~" = the field did not exist).
 * An implementation that lacks the later stages undoes them on the expected state, last stage first.
 *
 * An I line (only in recordings with a cheating pilot) lists fields the test harness set between the
 * preceding frame and the next one: a replaying implementation sets them in its state before running the
 * next frame. The next T line is still a delta against the preceding T line (the state before injection).
 */
class ReplayWriter(file: File, meta: Map<String, String>) : AutoCloseable {
    private val out: Writer = file.also { it.parentFile.mkdirs() }.bufferedWriter()
    private var previous: Map<String, String> = emptyMap()

    var ticks = 0L
        private set

    init {
        out.write("UGR 0\n")
        out.write("# UGH! golden replay - state after every frame, recorded in lockstep with the original\n")
        out.write("meta " + meta.entries.joinToString(" ") { "${it.key}=${it.value}" } + "\n")
    }

    fun tick(tick: Long, wait: Int, keys: List<Int>, state: Map<String, String>) {
        val line = StringBuilder()
        line.append("T ").append(tick).append(" w=").append("%04x".format(wait))
        line.append(" k=").append(if (keys.isEmpty()) "-" else keys.joinToString(",") { "%02x".format(it) })
        line.append(" |")
        for ((k, v) in state) if (previous[k] != v) line.append(' ').append(k).append('=').append(v)
        for (k in previous.keys) if (k !in state) line.append(' ').append(k).append("=~")
        line.append('\n')
        out.write(line.toString())
        previous = state
        ticks++
    }

    /** What [stage] of the frame of the last [tick] changed outside its own group: values before it. */
    fun before(stage: String, fields: Map<String, String>) {
        if (fields.isEmpty()) return
        out.write("B $stage " + fields.entries.joinToString(" ") { "${it.key}=${it.value}" } + "\n")
    }

    /** Fields set between the last [tick] and the next frame. */
    fun inject(fields: Map<String, String>) {
        if (fields.isEmpty()) return
        out.write("I " + fields.entries.joinToString(" ") { "${it.key}=${it.value}" } + "\n")
    }

    override fun close() = out.close()
}

/** Reads a "UGR 0" file back into full states per tick (used to check the writer and by later runners). */
object ReplayReader {
    data class Tick(val tick: Long, val wait: Int, val keys: List<Int>, val state: Map<String, String>,
                    var inject: Map<String, String> = emptyMap(),
                    val before: MutableMap<String, Map<String, String>> = linkedMapOf())

    fun read(file: File): Pair<Map<String, String>, List<Tick>> {
        val lines = file.readLines()
        require(lines.firstOrNull() == "UGR 0") { "not a UGR 0 file: $file" }
        var meta = emptyMap<String, String>()
        val ticks = ArrayList<Tick>()
        val state = sortedMapOf<String, String>()
        for (line in lines.drop(1)) {
            when {
                line.startsWith("#") || line.isBlank() -> {}
                line.startsWith("meta ") -> meta = pairs(line.removePrefix("meta "))
                line.startsWith("T ") -> {
                    val (head, body) = line.split(" |", limit = 2).let { it[0] to it.getOrElse(1) { "" } }
                    val h = head.split(' ')
                    val tick = h[1].toLong()
                    val hm = pairs(h.drop(2).joinToString(" "))
                    for ((k, v) in pairs(body)) if (v == "~") state.remove(k) else state[k] = v
                    val keys = hm.getValue("k").let { if (it == "-") emptyList() else it.split(',').map { s -> s.toInt(16) } }
                    ticks += Tick(tick, hm.getValue("w").toInt(16), keys, HashMap(state))
                }
                line.startsWith("B ") -> line.removePrefix("B ").split(" ", limit = 2).let { ticks.last().before[it[0]] = pairs(it.getOrElse(1) { "" }) }
                line.startsWith("I ") -> ticks.last().inject = pairs(line.removePrefix("I "))
                else -> error("unexpected line: $line")
            }
        }
        return meta to ticks
    }

    private fun pairs(s: String): Map<String, String> =
        s.trim().split(' ').filter { it.isNotEmpty() }.associate { it.substringBefore('=') to it.substringAfter('=') }
}
