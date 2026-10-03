package ugh.extractor

import java.io.File

/*
 * The game data for the C++ logic, format "UGD 1" (described in re/notes/phase2-data.md): text, one record per line,
 * every value already in the units the game computes in (PC pixels, 70 Hz frames, Fixed = 1/32 px as an integer)
 * and every reference by name (Names). Nothing of the original's memory layout is left in it; the arithmetic the
 * original does when it loads a level (113b:3976 - Amiga rows to PC rows, positions from the pads, the walker's
 * speed, the blower's height, the route delays, the water ceiling) is done here.
 */

/** One line of a UGD file: `<type> [<name>] <key>=<value> ...`. */
data class UgdRecord(val type: String, val name: String?, val fields: Map<String, String>) {
    operator fun get(key: String): String = fields[key] ?: error("$type ${name ?: ""}: no $key")
    fun int(key: String) = get(key).toInt()
    fun ints(key: String) = get(key).let { if (it == "-") emptyList() else it.split(',').map(String::toInt) }

    override fun toString() = buildString {
        append(type)
        if (name != null) append(' ').append(name)
        for ((k, v) in fields) append(' ').append(k).append('=').append(v)
    }
}

object LogicData {
    const val MAGIC = "UGD 1"

    private const val D = Layout.DGROUP

    // tables of the original's data segment
    private const val LEVEL_COUNTS = 0x2624
    private const val MULTIPLIER_LIMITS = 0x2628
    private const val CRASH_LIMITS = 0x262e
    private const val ROTOR_SPRITES = 0x0cfa
    private const val KEY_TABLE = 0x281c
    private const val QUICK_DELIVERY_BONUS = 0x7a38
    private const val LIST_END = 0xffff

    // sprite numbers the code of the original has as constants
    private const val STANDING_PASSENGER = 0x220      // 113b:1c27
    private const val DROPPED_PASSENGER = 0x221       // 113b:1c81
    private const val BOUNCED_PASSENGER = 0x222       // 113b:2196, 2ab5
    private const val SHAKEN_TREE = 0xe8              // 113b:2b0c
    private const val DESTINATION_BUBBLES = 0x10c     // 113b:16f6: + the target pad, at most 0x111
    private const val LAST_DESTINATION_BUBBLE = 0x111
    private const val IMPATIENT_BUBBLE = 0x112        // 113b:17e6

    /** The kind with this descriptor cannot be rescued from the water (113b:1f43 checks its address). */
    private const val UNRESCUABLE_SWIMMER = 0x77fe

    /**
     * The state handlers a descriptor names in its slots +08 .. (113b:xxxx). The C++ logic has the state machines
     * hard-wired, so the data must name exactly these; 0 = a slot nobody jumps to.
     */
    private val ROUTE_SLOTS = listOf(0x149c, 0x1509, 0x153b, 0x1582, 0x15b4, 0x15d7, 0x16f6, 0x172a, 0x17e6, 0x180a,
        0x18c8, 0x18e6, 0x19fb, 0x1a42, 0x1a7e, 0x1b29, 0x1bbe, 0x1bd6, 0x1c0e)
    private val WATER_SLOTS = listOf(0x1da8, 0x1dd5, 0x1e9c, 0x1ec0, 0x1f24, 0x1f43, 0x16f6, 0x1fe2, 0x17e6, 0x2068,
        0x18c8, 0x20c1, 0x19e0, 0, 0, 0, 0, 0, 0x1c0e)
    private val STANDING_SLOTS = listOf(0, 0, 0, 0, 0x1c0f, 0x1c27, 0, 0, 0, 0, 0, 0, 0x1c48, 0x1c6b, 0x1c81, 0x1cee,
        0, 0, 0x1c0e)
    private val ENEMY_SLOTS = mapOf(
        Names.FLYER to listOf(0x2379, 0x239f, 0x23b0, 0x23d9, 0x23ea, 0x2493, 0, 0, 0, 0, 0x252b, 0x255e, 0x1c0e),
        Names.WALKER to listOf(0x25b1, 0x25c9, 0x2667, 0x2681, 0x272e, 0x2748, 0x2830, 0x2844, 0x288f, 0, 0x28ee,
            0x2914, 0x1c0e),
        Names.BLOWER to listOf(0, 0, 0, 0, 0x295b, 0x2973, 0, 0, 0, 0, 0x2a53, 0x2a76, 0x1c0e),
        Names.TREE to listOf(0x2a87, 0x2ab5, 0, 0, 0, 0, 0, 0, 0, 0, 0x2b0c, 0x2b58, 0x2b7e),
    )
    private val BONUS_SLOTS = listOf(0x2b96, 0x2be4, 0x2c97, 0x2ca9)

    private val KEYS = listOf("up", "down", "left", "right", "fire")

    /** Every record of the data, in file order. */
    fun records(exe: Exe): List<UgdRecord> = Builder(exe).build()

    fun write(exe: Exe, file: File) {
        file.parentFile?.mkdirs()
        file.writeText(buildString {
            append(MAGIC).append('\n')
            append("# UGH! game data for the C++ logic, extracted from UGH.EXE sha256 ").append(sha256(exe.bytes)).append('\n')
            for (r in records(exe)) append(r).append('\n')
        })
    }

    /** Reads a UGD file back into its records (without the header and the comments). */
    fun read(file: File): List<UgdRecord> {
        val lines = file.readLines()
        require(lines.firstOrNull() == MAGIC) { "not a $MAGIC file: $file" }
        return lines.drop(1).filter { it.isNotBlank() && !it.startsWith("#") }.map { line ->
            val words = line.split(' ')
            val named = words.size > 1 && '=' !in words[1]
            val fields = linkedMapOf<String, String>()
            for (w in words.drop(if (named) 2 else 1)) fields[w.substringBefore('=')] = w.substringAfter('=')
            UgdRecord(words[0], if (named) words[1] else null, fields)
        }
    }

    // ------------------------------------------------------------------ the arithmetic of the level load

    /** A word as the 8086 keeps it: signed 16 bits. */
    fun s16(v: Int) = v.toShort().toInt()

    /** 113b:3d4d - Amiga rows to PC rows (256 to 192 lines): y - (y SAR 2). */
    fun amigaRowsToPc(y: Int) = s16(y - (s16(y) shr 2))

    /** 113b:3d5a - Amiga frames to PC frames (50 to 70 Hz, roughly): n + (n SAR 1). */
    fun amigaFramesToPc(n: Int) = s16(n + (s16(n) shr 1))

    /** Whole pixels to Fixed (SHL 5, wraps). */
    fun fixed(pixels: Int) = s16(pixels shl 5)

    private class Builder(private val exe: Exe) {
        private fun u(o: Int) = exe.u16(D, o)
        private fun w(o: Int) = exe.s16(D, o)
        private fun b(o: Int) = exe.u8(D, o)

        private val animations = linkedMapOf<Int, String>()   // offset -> name, in the order of first use
        private val passengerKinds = sortedSetOf<Int>()
        private val enemyKinds = sortedSetOf<Int>()
        private val bonusKinds = sortedSetOf<Int>()

        fun build(): List<UgdRecord> {
            val set = Levels.extract(exe)
            val counts = listOf(u(LEVEL_COUNTS), u(LEVEL_COUNTS + 2))
            check(counts == listOf(set.onePlayer.size, set.team.size)) { "level counts $counts" }
            val levels = set.levels.withIndex().flatMap { (id, level) -> level(level, id) }
            bonusKinds += QUICK_DELIVERY_BONUS
            // the walking kinds bring their swimming twins
            for (k in passengerKinds.toList()) if (typeOf(k) == "route") passengerKinds += u(k + 0x48)
            val kinds = passengerKinds.map { passengerKind(it) } + enemyKinds.map { enemyKind(it) } + bonusKinds.map { bonusKind(it) }
            val records = ArrayList<UgdRecord>()
            records += rules()
            records += sprites()
            records += keys()
            records += animations.map { (offset, name) -> rec("animation", name, "frames" to frames(offset).joinToString(",")) }
            records += kinds
            records += levels
            records += rec("order", null, "oneplayer" to set.onePlayer.joinToString(",") { set.levels.indexOfFirst { l -> l.record == it }.toString() })
            records += rec("order", null, "team" to set.team.joinToString(",") { set.levels.indexOfFirst { l -> l.record == it }.toString() })
            return records
        }

        private fun rec(type: String, name: String?, vararg fields: Pair<String, Any>) =
            UgdRecord(type, name, fields.associate { it.first to it.second.toString() }.let { LinkedHashMap(it) })

        private fun rules() = rec("rules", null,
            "crashLimit" to (0..2).joinToString(",") { w(CRASH_LIMITS + 2 * it).toString() },
            "multiplierLimit" to (0..2).joinToString(",") { w(MULTIPLIER_LIMITS + 2 * it).toString() },
            "quickDeliveryBonus" to Names.bonusKind(QUICK_DELIVERY_BONUS))

        private fun sprites(): UgdRecord {
            val rotor = (0..2).map { u(ROTOR_SPRITES + 2 * it) }
            return rec("sprites", null,
                "standingPassenger" to STANDING_PASSENGER, "droppedPassenger" to DROPPED_PASSENGER,
                "bouncedPassenger" to BOUNCED_PASSENGER, "shakenTree" to SHAKEN_TREE,
                "destinationBubbles" to "$DESTINATION_BUBBLES..$LAST_DESTINATION_BUBBLE", "impatientBubble" to IMPATIENT_BUBBLE,
                "rotor0" to "${rotor[0]}..${rotor[1] - 1}", "rotor1" to "${rotor[1]}..${rotor[2] - 1}")
        }

        /** The key table (DGROUP:281c, 6 bytes per key up to 0xff): scancodes, then the key state it sets. */
        private fun keys(): List<UgdRecord> {
            val keys = ArrayList<UgdRecord>()
            var entry = KEY_TABLE
            while (b(entry) != 0xff) {
                val codes = if (b(entry + 1) == 0xff) listOf(b(entry)) else listOf(b(entry), b(entry + 1))
                check(b(entry + 2) == 0xff) { "key table: a sequence of three scancodes" }
                val slot = b(entry + 3)   // the offset of the key state in 278c: 4 per key, 2 per player
                val press = b(entry + 4)
                check(press == 0 || press == 0xff) { "key table: a key state other than 0 / 0xff" }
                keys += when {
                    slot == 0x14 -> rec("key", null, "codes" to codes.joinToString(","), "key" to "none")
                    slot < 0x14 && slot % 2 == 0 -> rec("key", null, "codes" to codes.joinToString(","),
                        "player" to ((slot / 2) and 1), "key" to KEYS[slot / 4], "press" to if (press == 0xff) 1 else 0)
                    else -> error("key table: unknown key ${slot.hex()}")
                }
                entry += 6
            }
            return keys
        }

        private fun frames(offset: Int): List<Int> {
            val frames = ArrayList<Int>()
            var a = offset
            while (u(a) != LIST_END) { frames += u(a); a += 2; check(frames.size < 100) { "animation ${offset.hex()} has no end" } }
            check(frames.isNotEmpty()) { "animation ${offset.hex()} is empty" }
            return frames
        }

        private fun animation(offset: Int, name: String): String {
            check(offset != 0) { "$name: no animation" }
            return animations.getOrPut(offset) { name }
        }

        private fun box(d: Int) = "${w(d)},${w(d + 2)},${w(d + 4)},${w(d + 6)}"

        private fun checkSlots(d: Int, slots: List<Int>, what: String) {
            for ((i, handler) in slots.withIndex())
                check(handler == 0 || u(d + 8 + 2 * i) == handler) { "$what ${d.hex()}: slot +${(8 + 2 * i).hex()} is not 113b:${handler.hex()}" }
        }

        private fun typeOf(d: Int) = when (u(d + 8)) {
            ROUTE_SLOTS[0] -> "route"
            WATER_SLOTS[0] -> "water"
            else -> "standing"
        }

        private fun passengerKind(d: Int): UgdRecord {
            val name = Names.passengerKind(d)
            fun anim(slot: Int) = animation(u(d + slot), "$name.${Names.PASSENGER_ANIMATIONS.getValue(slot)}")
            val f = linkedMapOf<String, Any>("type" to typeOf(d), "box" to box(d))
            when (typeOf(d)) {
                "route" -> {
                    checkSlots(d, ROUTE_SLOTS, "walking passenger")
                    check(typeOf(u(d + 0x48)) == "water") { "passenger ${d.hex()}: +48 is not its swimming kind" }
                    f["standing"] = anim(0x2e); f["waving"] = anim(0x30); f["walk"] = "${anim(0x32)},${anim(0x34)}"
                    f["comingOut"] = anim(0x36); f["goingIn"] = anim(0x38)
                    f["animDelay"] = w(d + 0x3e); f["fare"] = w(d + 0x40); f["fareMin"] = w(d + 0x42); f["look"] = w(d + 0x46)
                    f["waterKind"] = Names.passengerKind(u(d + 0x48))
                }
                "water" -> {
                    checkSlots(d, WATER_SLOTS, "swimming passenger")
                    check(typeOf(u(d + 0x48)) == "route") { "passenger ${d.hex()}: +48 is not its walking kind" }
                    f["standing"] = anim(0x2e); f["waving"] = anim(0x30); f["walk"] = "${anim(0x32)},${anim(0x34)}"
                    f["animDelay"] = w(d + 0x3e); f["fare"] = w(d + 0x40); f["fareMin"] = w(d + 0x42); f["swimTime"] = w(d + 0x44)
                    f["landKind"] = Names.passengerKind(u(d + 0x48)); f["rescuable"] = if (d == UNRESCUABLE_SWIMMER) 0 else 1
                }
                else -> {
                    checkSlots(d, STANDING_SLOTS, "standing passenger")
                    f["look"] = w(d + 0x46)
                }
            }
            return UgdRecord("passengerKind", name, f.mapValues { it.value.toString() })
        }

        private fun enemyKind(d: Int): UgdRecord {
            val type = Names.enemyKind(d)
            checkSlots(d, ENEMY_SLOTS.getValue(d), type)
            fun anim(slot: Int, name: String = Names.ENEMY_ANIMATIONS.getValue(slot)) = animation(u(d + slot), "$type.$name")
            fun pair(slot: Int) = "${anim(slot)},${anim(slot + 2)}"
            fun firstFrame(slot: Int) = frames(u(d + slot)).first()
            return when (d) {
                Names.FLYER -> rec("flyerKind", null, "box" to box(d), "flight" to pair(0x22),
                    "hitSprite" to "${firstFrame(0x36)},${firstFrame(0x38)}", "score" to w(d + 0x3a))
                Names.BLOWER -> rec("blowerKind", null, "box" to box(d), "blowing" to anim(0x26, "blowing"),
                    "stunnedSprite" to firstFrame(0x36), "score" to w(d + 0x3a))
                Names.TREE -> rec("treeKind", null, "swaying" to anim(0x22, "swaying"))
                else -> rec("walkerKind", null, "box" to box(d), "walk" to pair(0x22), "watch" to pair(0x2e),
                    "charge" to pair(0x26), "recover" to pair(0x2a), "stunned" to pair(0x36), "score" to w(d + 0x3a))
            }
        }

        private fun bonusKind(d: Int): UgdRecord {
            for ((i, handler) in BONUS_SLOTS.withIndex()) check(u(d + 4 + 2 * i) == handler) { "bonus item ${d.hex()}: unexpected state handlers" }
            val effect = when (u(d + 0x12)) { 0 -> "energy"; 1 -> "life"; 2 -> "multiplier"; else -> error("bonus item ${d.hex()}: unknown effect") }
            return rec("bonusKind", Names.bonusKind(d), "effect" to effect, "amount" to w(d + 0x0e), "lift" to w(d + 0x0c),
                "sprite" to u(d + 0x10), "anchor" to "${w(d)},${w(d + 2)}")
        }

        /** 113b:3976 - a level record (26 bytes) and its lists A (pads), B (passengers), C (enemies), and its mask. */
        private fun level(level: Level, id: Int): List<UgdRecord> {
            val r = level.record
            val wind = b(r + 0x0c)
            check(wind <= 2) { "level ${r.hex()}: unknown wind" }
            var water = amigaRowsToPc(w(r + 0x15))
            if ((water and 0xffff) > 0x17e0) water = 0x17e0   // compared unsigned
            val out = ArrayList<UgdRecord>()
            out += rec("level", id.toString(), "toDeliver" to b(r + 8), "wind" to listOf("none", "left", "right")[wind],
                "start0" to start(r, 0), "start1" to start(r, 1), "water" to water, "waterSpeed" to w(r + 0x17))
            val pads = pads(u(r + 2))
            for (p in pads) out += rec("pad", null, "left" to p[0], "right" to p[1], "y" to p[2], "door" to p[3],
                "wait" to p[4], "stand" to p[5], "number" to p[6])
            fun pad(i: Int) = pads.getOrNull(i) ?: error("level ${r.hex()}: no pad $i")
            out += passengers(u(r + 4), ::pad)
            out += enemies(u(r + 6), ::pad)
            out += mask(level).map { rec("mask", null).copy(name = it) }
            return out
        }

        private fun start(r: Int, player: Int) =
            "${s16(w(r + 0x0d + 4 * player) - 0x200)},${s16(amigaRowsToPc(w(r + 0x0f + 4 * player)) - 0x140)}"

        /** List A: 8 words per pad up to a negative one; left, right, y (Amiga rows), door, wait, stand, number. */
        private fun pads(list: Int): List<List<Int>> {
            val pads = ArrayList<List<Int>>()
            var e = list
            while (w(e) >= 0) {
                pads += listOf(w(e), w(e + 2), amigaRowsToPc(w(e + 4)), w(e + 6), w(e + 8), w(e + 10), w(e + 12))
                e += 16
            }
            return pads
        }

        /** List B: kind, pad, route (3 words) up to 0xffff. */
        private fun passengers(list: Int, pad: (Int) -> List<Int>): List<UgdRecord> {
            val out = ArrayList<UgdRecord>()
            var e = list
            while (u(e) != LIST_END) {
                val kind = u(e)
                passengerKinds += kind
                when (typeOf(kind)) {
                    "standing" -> {
                        check(u(e + 4) == 0) { "a standing passenger with a route" }
                        val p = pad(w(e + 2))
                        out += rec("standingPassenger", null, "x" to fixed(p[5] - 8), "y" to fixed(p[2] - 0xb))
                    }
                    "route" -> out += rec("routePassenger", null, "kind" to Names.passengerKind(kind), "route" to route(u(e + 4), pad))
                    else -> error("a swimming passenger in a level")
                }
                e += 6
            }
            return out
        }

        /**
         * A route: (pad, delay in Amiga frames) pairs up to 0xffff in the place of a pad. Stop n goes from pad n after
         * its delay to pad n + 1; the delay of the last pad is never used.
         */
        private fun route(list: Int, pad: (Int) -> List<Int>): String {
            val entries = ArrayList<Pair<Int, Int>>()
            var a = list
            while (u(a) != LIST_END) {
                pad(w(a))
                entries += w(a) to w(a + 2)
                a += 4
                check(entries.size < 32) { "route ${list.hex()} has no end" }
            }
            check(entries.size >= 2) { "route ${list.hex()} has no stop" }
            return (entries.dropLast(1).map { (p, delay) -> "$p/${amigaFramesToPc(delay)}" } + "${entries.last().first}").joinToString(",")
        }

        /** List C: the descriptor, then 2 to 4 words by type, up to 0xffff. */
        private fun enemies(list: Int, pad: (Int) -> List<Int>): List<UgdRecord> {
            val out = ArrayList<UgdRecord>()
            var e = list
            while (u(e) != LIST_END) {
                val kind = u(e)
                enemyKinds += kind
                when (kind) {
                    Names.TREE -> {
                        val p = pad(w(e + 2))
                        val drops = ArrayList<String>()
                        var a = u(e + 4)
                        while (u(a) != LIST_END) { bonusKinds += u(a); drops += Names.bonusKind(u(a)); a += 2 }
                        out += rec("tree", null, "x" to fixed(p[5] - 0x10), "y" to fixed(p[2] - 0x18),
                            "drops" to drops.joinToString(",").ifEmpty { "-" })
                        e += 6
                    }
                    Names.FLYER -> {
                        out += rec("flyer", null, "startDelay" to amigaFramesToPc(w(e + 4)), "speed" to w(e + 6))
                        e += 8
                    }
                    Names.BLOWER -> {
                        out += rec("blower", null, "x" to fixed(w(e + 2)), "y" to s16(amigaRowsToPc(fixed(w(e + 4))) - 0x2c0))
                        e += 10
                    }
                    else -> {
                        check(kind == Names.WALKER) { "an unknown enemy kind ${kind.hex()}" }
                        val i = w(e + 2)
                        val p = pad(i)
                        val speed = w(e + 6)
                        out += rec("walker", null, "pad" to i, "x" to fixed(p[5] - 0x10), "y" to fixed(p[2] - 0x16),
                            "speed" to s16((speed shr 2) - speed))
                        e += 8
                    }
                }
            }
            return out
        }

        /**
         * The collision mask (Masks): 192 rows of 320 px as 80 hex digits, the leftmost pixel in the highest bit.
         * The original's background page is 384 px wide, but the tiles cover only 0..319: the rest must be empty.
         */
        private fun mask(level: Level): List<String> {
            val mask = Masks.mask(level, sprites)
            return (0 until Masks.HEIGHT).map { y ->
                for (x in MASK_WIDTH until Masks.WIDTH) check(!Masks.bit(mask, x, y)) { "level ${level.record.hex()}: solid pixel at x $x" }
                (0 until MASK_WIDTH / 8).joinToString("") { "%02x".format(mask[y * Masks.ROW_BYTES + it].toInt() and 0xff) }
            }
        }

        private val sprites by lazy { Sprites.table(exe).map { if (it.width > 0 && it.height > 0) Sprites.decode(exe, it) else null } }
    }

    /** The width of the collision mask in UGD 1 (the screen). */
    const val MASK_WIDTH = 320
}
