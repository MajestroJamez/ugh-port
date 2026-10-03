package ugh.verify.replay

import ugh.core.game.Game
import ugh.extractor.Names

/**
 * Projection of the original's memory (port or oracle) onto the semantic game state of the replays "UGR 1": the
 * concepts of the game by name, the units of the game (positions in 1/32 px, speeds in 1/64 of that per frame, frames,
 * sprite numbers), no address and no memory layout. This is what the C++ logic (logic/) reproduces tick by tick.
 *
 * A field is in the state exactly when it is defined: the game wrote it and reads it later, or a renderer needs it.
 * What the original keeps only as a leftover of earlier memory use is not in it. The rules are the tables below,
 * by entity type and state ([PASSENGER_FIELDS] ...), as measured in step N1 (re/notes/rewrite-audit.md); the C++
 * StateWriter has the same rules, and the replay check compares the set of fields as well as the values.
 */
class SemanticProjection(private val read16: (seg: Int, off: Int) -> Int) {

    private fun u(off: Int) = read16(Game.DGROUP, off) and 0xffff
    private fun s(off: Int) = Game.s16(u(off))
    private fun b(off: Int) = read16(Game.DGROUP, off) and 0xff
    private fun cs(off: Int) = read16(Game.CODE, off) and 0xffff

    fun project(phase: String): Map<String, String> {
        val m = sortedMapOf<String, String>()
        game(m, phase)
        if (phase !in LEVEL_PHASES) return m
        val players = u(0x2634).coerceIn(1, 2)
        for (p in 0 until players) copter(m, p)
        val record = u(u(0x2909 + ((players - 1) shl 1)) + (u(0x261c) shl 1))
        for (i in 0 until 10) { if (s(Game.PAD_LEFT + 2 * i) < 0) break; pad(m, i) }
        var bEntry = u(record + 4)
        for (i in 0 until 16) { if (u(0x29ad + 2 * i) == 0xffff) break; passenger(m, i, bEntry); bEntry += 6 }
        var cEntry = u(record + 6)
        for (i in 0 until 5) {
            val kind = u(0x2cad + 2 * i)
            if (kind == 0xffff) break
            enemy(m, i, cEntry)
            cEntry += when (kind) { Names.TREE -> 6; Names.BLOWER -> 10; else -> 8 }
        }
        for (i in 0 until 12) if (u(0x2d9b + 2 * i) != 0xffff) bonus(m, i)
        return m
    }

    // ---------------------------------------------------------------- game

    private fun game(m: MutableMap<String, String>, phase: String) {
        m["game.phase"] = phase
        m["game.level"] = u(0x261c).toString()
        m["game.players"] = u(0x2634).toString()
        m["game.difficulty"] = DIFFICULTIES.getOrElse(u(0x2638)) { "?" + u(0x2638) }
        m["game.lives"] = b(0x263c).toString()
        m["game.multiplier"] = b(0x263d).toString()
        m["game.score"] = ((u(0x2620).toLong() shl 16) or u(0x261e).toLong()).toString()
        m["game.rng"] = "%04x%04x%04x%04x".format(cs(0x4efd), cs(0x4efb), cs(0x4ef9), cs(0x4ef7))
        m["game.rainFloor"] = (u(0x2907) / ROW_BYTES).toString()
        if (phase !in LEVEL_PHASES) return
        m["game.energy"] = s(Game.V_ENERGY).toString()
        m["game.fade"] = s(Game.V_FADE).toString()
        m["game.fadeDirection"] = if (s(Game.V_FADE_STEP) < 0) "out" else "in"
        m["game.levelDone"] = flag(b(0x27cf) and 0x80 != 0)
        m["game.wind"] = WINDS.getOrElse(b(Game.V_WIND)) { "?" + b(Game.V_WIND) }
        m["game.passengersLeft"] = b(0x28f1).toString()
        m["game.water.level"] = s(0x28fe).toString()
        m["game.water.resting"] = flag(b(0x27a2) != 0)
        m["game.water.evenFrame"] = b(0x27ce).toString()
        m["game.water.surfaceFrame"] = s(0x27a4).toString()
        m["game.water.surfaceDelay"] = b(0x27a3).toString()
        m["game.rain"] = if (b(Game.V_WIND) != 0) rainChecksum() else "none"
    }

    /** CRC-32 (zlib) of the 193 raindrops 0..192 as little-endian int16 pairs x, y (the page is 384 px wide). */
    private fun rainChecksum(): String {
        val crc = java.util.zip.CRC32()
        fun word(v: Int) { crc.update(v and 0xff); crc.update((v shr 8) and 0xff) }
        for (bx in 0..0x180 step 2) {
            val offset = u(0x2e8b + bx)   // row * 96 + x / 4 in the VGA page, the plane is x & 3
            word((offset % ROW_BYTES) * 4 + (u(0x318f + bx) and 3))
            word(offset / ROW_BYTES)
        }
        return "%08x".format(crc.value)
    }

    // ---------------------------------------------------------------- copters

    private fun copter(m: MutableMap<String, String>, p: Int) {
        val bx = 2 * p
        val c = "copter.$p."
        m[c + "x"] = s(Game.P_XF + bx).toString()
        m[c + "y"] = s(Game.P_YF + bx).toString()
        m[c + "pixelX"] = s(Game.P_X + bx).toString()
        m[c + "pixelY"] = s(Game.P_Y + bx).toString()
        m[c + "vx"] = s(Game.P_VX + bx).toString()
        m[c + "vy"] = s(Game.P_VY + bx).toString()
        m[c + "landedPad"] = s(Game.P_LANDED + bx).let { if (it < 0) "none" else it.toString() }
        m[c + "rotorSprite"] = u(0x27e8 + bx).toString()
        m[c + "rotorCounter"] = s(0x27f0 + bx).toString()
        m[c + "keys"] = buildString {
            if (u(Game.KEY_UP + bx) != 0) append('U')
            if (u(Game.KEY_DOWN + bx) != 0) append('D')
            if (u(Game.KEY_LEFT + bx) != 0) append('L')
            if (u(Game.KEY_RIGHT + bx) != 0) append('R')
            if (u(0x279c + bx) != 0) append('F')
            if (isEmpty()) append('-')
        }
        m[c + "cargoLook"] = u(0x27fc + bx).let { if (it == 0) "none" else it.toString() }
        val destination = s(0x2804 + bx)
        m[c + "destination"] = when (destination) { 0 -> "none"; HANGING_DESTINATION -> "hanging"; else -> destination.toString() }
        m[c + "fare"] = s(0x2808 + bx).toString()
        m[c + "effort"] = s(Game.P_EFFORT + bx).toString()
        // the minimum fare is defined while a passenger with a route rides (its destination is a pad)
        if (destination != 0 && destination != HANGING_DESTINATION) m[c + "fareMin"] = s(0x280c + bx).toString()
    }

    // ---------------------------------------------------------------- pads

    private fun pad(m: MutableMap<String, String>, i: Int) {
        val si = 2 * i
        val c = "pad.$i."
        m[c + "left"] = s(Game.PAD_LEFT + si).toString()
        m[c + "right"] = s(Game.PAD_RIGHT + si).toString()
        m[c + "y"] = s(Game.PAD_Y + si).toString()
        m[c + "door"] = s(0x2949 + si).toString()
        m[c + "wait"] = s(0x295d + si).toString()
        m[c + "stand"] = s(0x2971 + si).toString()
        m[c + "number"] = s(0x2985 + si).toString()
        m[c + "waiting"] = s(0x2999 + si).let { if (it < 0) "none" else it.toString() }
    }

    // ---------------------------------------------------------------- passengers

    private fun passenger(m: MutableMap<String, String>, i: Int, entry: Int) {
        val si = 2 * i
        val kind = u(0x29ad + si)
        val handler = u(0x2a0d + si)
        val standing = kind == STANDING_KIND
        val state = (if (standing) STANDING_STATES[handler] else ROUTE_STATES[handler]) ?: ("?%04x".format(handler))
        val counter = s(0x2a2d + si)
        val all = mapOf(
            "kind" to { Names.passengerKind(kind) },
            "state" to { state },
            "sprite" to { noneIf(u(0x2bed + si)) },
            "bubble" to { noneIf(u(0x2c2d + si)) },
            "x" to { s(0x2aed + si).toString() },
            "y" to { s(0x2b0d + si).toString() },
            "routeStop" to { ((u(0x29ed + si) - u(entry + 4)) / 4).toString() },
            "pickupPad" to { s(0x2a4d + si).toString() },
            "targetPad" to { s(0x2a6d + si).toString() },
            "seenX" to { s(0x2b2d + si).toString() },
            "seenY" to { s(0x2b6d + si).toString() },
            "animFrame" to { (s(0x2bad + si) shr 1).toString() },
            "animDelay" to { s(0x2bcd + si).toString() },
            "arrivalDelay" to { s(0x2acd + si).toString() },
            "callTime" to { counter.toString() },
            "waitingSpot" to { when (counter) { -1 -> "starting"; 0 -> "walking"; 1 -> "reached"; else -> "?$counter" } },
            "carrier" to { (counter / 2).toString() },
            "quickDeliveryTime" to { s(0x2c8d + si).toString() },
            "dropSpeedX" to { s(0x2acd + si).toString() },
            "fallSpeed" to { s(0x2c6d + si).toString() },
            "swimSpeed" to { s(0x2c6d + si).toString() },
            "swimTime" to { s(0x2acd + si).toString() },
        )
        val fields = (if (standing) STANDING_FIELDS else ROUTE_FIELDS)[state] ?: setOf("kind", "state")
        for (f in fields) m["passenger.$i.$f"] = all.getValue(f)()
    }

    // ---------------------------------------------------------------- enemies

    private fun enemy(m: MutableMap<String, String>, i: Int, entry: Int) {
        val si = 2 * i
        val descriptor = u(0x2cad + si)
        val type = Names.enemyKind(descriptor)
        val handler = u(0x2cf3 + si)
        val state = ENEMY_STATES.getValue(type)[handler] ?: ("?%04x".format(handler))
        val timer = s(0x2cfd + si)
        val all = mapOf(
            "kind" to { type },
            "state" to { state },
            "sprite" to { noneIf(u(0x2d43 + si)) },
            "x" to { s(0x2cc1 + si).toString() },
            "y" to { s(0x2ccb + si).toString() },
            "animFrame" to { (s(0x2d2f + si) shr 1).toString() },
            "animDelay" to { s(0x2d39 + si).toString() },
            "vx" to { s(0x2ce9 + si).toString() },
            "lastTarget" to { s(0x2d57 + si).toString() },
            "facing" to { when (s(0x2d57 + si)) { 0 -> "left"; 2 -> "right"; else -> "?" + s(0x2d57 + si) } },
            "flight" to {
                when (u(0x2cd5 + si)) { u(descriptor + 0x22) -> "left"; u(descriptor + 0x24) -> "right"; else -> "?%04x".format(u(0x2cd5 + si)) }
            },
            "nextDrop" to { ((u(0x2cd5 + si) - u(entry + 4)) / 2).toString() },
            "waitTime" to { timer.toString() },
            "screechTime" to { timer.toString() },
            "fallSpeed" to { timer.toString() },
            "watchTime" to { timer.toString() },
            "chargeSpeed" to { timer.toString() },
            "stunTime" to { timer.toString() },
            "restTime" to { timer.toString() },
        )
        val fields = ENEMY_FIELDS.getValue(type)[state] ?: setOf("kind", "state")
        for (f in fields) m["enemy.$i.$f"] = all.getValue(f)()
    }

    // ---------------------------------------------------------------- bonus items

    private fun bonus(m: MutableMap<String, String>, i: Int) {
        val si = 2 * i
        val handler = u(0x2d83 + si)
        val state = BONUS_STATES[handler] ?: ("?%04x".format(handler))
        val all = mapOf(
            "kind" to { Names.bonusKind(u(0x2d6b + si)) },
            "state" to { state },
            "x" to { s(0x2dcb + si).toString() },
            "y" to { s(0x2de3 + si).toString() },
            "sprite" to { u(0x2d9b + si).toString() },
            "vx" to { s(0x2e5b + si).toString() },
            "vy" to { s(0x2e73 + si).toString() },
            "lyingTime" to { s(0x2e5b + si).toString() },
        )
        val fields = BONUS_FIELDS[state] ?: setOf("kind", "state")
        for (f in fields) m["bonus.$i.$f"] = all.getValue(f)()
    }

    companion object {
        /** Phases in which a level is loaded: its entities are in the state. */
        val LEVEL_PHASES = setOf("caption", "setup", "play")

        private const val ROW_BYTES = 0x60          // the background page: 384 px = 96 bytes per row and plane
        private const val HANGING_DESTINATION = 7   // 113b:1c48: the pad number shown while the standing passenger hangs
        private const val STANDING_KIND = 0x78dc

        private val DIFFICULTIES = listOf("easy", "medium", "hard")
        private val WINDS = listOf("none", "left", "right")

        private fun flag(v: Boolean) = if (v) "1" else "0"
        private fun noneIf(sprite: Int) = if (sprite == 0xffff) "none" else sprite.toString()

        private fun fields(vararg groups: Pair<String, String>): Map<String, Set<String>> {
            val m = HashMap<String, MutableSet<String>>()
            for ((states, list) in groups) for (st in states.split(' ')) m.getOrPut(st) { linkedSetOf() } += list.split(' ')
            return m
        }

        /** The state handlers a passenger is in at the end of a frame, by address: the name of the state. */
        val ROUTE_STATES = mapOf(
            0x149c to "NextStop", 0x1509 to "BehindDoor", 0x1582 to "ComingOut", 0x15d7 to "Waiting", 0x172a to "Calling",
            0x180a to "Impatient", 0x18e6 to "Boarding", 0x1a42 to "Riding", 0x1b29 to "WalkingToDoor", 0x1bd6 to "GoingIn",
            0x1c0e to "Gone", 0x1dd5 to "Splash", 0x1ec0 to "Sinking", 0x1f43 to "Swimming", 0x1fe2 to "SwimCalling",
            0x2068 to "SwimWaving", 0x20c1 to "SwimBoarding",
        )
        val STANDING_STATES = mapOf(0x1c0f to "Placed", 0x1c27 to "Standing", 0x1c6b to "Hanging", 0x1cee to "Falling", 0x1c0e to "Gone")
        val ENEMY_STATES = mapOf(
            "flyer" to mapOf(0x2379 to "Placed", 0x239f to "Hidden", 0x23d9 to "Screeching", 0x2493 to "Flying", 0x255e to "Falling"),
            "walker" to mapOf(0x25b1 to "Placed", 0x25c9 to "Walking", 0x2681 to "Watching", 0x2748 to "Charging",
                0x2844 to "Recovering", 0x2914 to "Stunned"),
            "blower" to mapOf(0x295b to "Placed", 0x2973 to "Blowing", 0x2a76 to "Stunned"),
            "tree" to mapOf(0x2a87 to "Placed", 0x2ab5 to "Swaying", 0x2b58 to "Resting", 0x2b7e to "Bare"),
        )
        val BONUS_STATES = mapOf(0x2be4 to "Falling", 0x2ca9 to "Lying")

        private const val ROUTE_ALL = "NextStop BehindDoor ComingOut Waiting Calling Impatient Boarding Riding " +
            "WalkingToDoor GoingIn Gone Splash Swimming SwimCalling SwimWaving SwimBoarding Sinking"
        private const val ROUTE_ANIMATED = "ComingOut Waiting Calling Impatient Boarding WalkingToDoor GoingIn Splash " +
            "Swimming SwimCalling SwimWaving SwimBoarding Sinking"

        /** The fields of a passenger with a route (and in the water) by state. */
        val ROUTE_FIELDS = fields(
            ROUTE_ALL to "kind state sprite bubble",
            "$ROUTE_ANIMATED" to "x y animFrame animDelay",
            "NextStop BehindDoor ComingOut Waiting Calling Impatient Boarding Riding WalkingToDoor GoingIn Splash Swimming " +
                "SwimCalling SwimWaving SwimBoarding Sinking" to "routeStop",
            "BehindDoor ComingOut Waiting Calling Impatient Boarding Riding WalkingToDoor GoingIn Splash Swimming " +
                "SwimCalling SwimWaving SwimBoarding Sinking" to "pickupPad targetPad",
            "$ROUTE_ANIMATED Riding" to "seenX seenY",
            "BehindDoor" to "arrivalDelay",
            "Calling Impatient SwimCalling SwimWaving" to "callTime",
            "Waiting" to "waitingSpot",
            "Riding" to "carrier quickDeliveryTime",
            "Splash Sinking" to "swimSpeed",
            "Swimming" to "swimTime",
        )

        /** The fields of the standing passenger by state. */
        val STANDING_FIELDS = fields(
            "Placed Standing Hanging Falling Gone" to "kind state sprite bubble",
            "Placed Standing Falling" to "x y",
            "Hanging" to "carrier",
            "Falling" to "dropSpeedX fallSpeed",
        )

        /** The fields of the enemies by type and state. */
        val ENEMY_FIELDS = mapOf(
            "flyer" to fields(
                "Placed Hidden Screeching Flying Falling" to "kind state vx lastTarget",
                "Hidden Screeching Flying Falling" to "sprite",
                "Flying Falling" to "x y",
                "Hidden Screeching Flying" to "animFrame animDelay",
                "Flying" to "flight", "Hidden" to "waitTime", "Screeching" to "screechTime", "Falling" to "fallSpeed",
            ),
            "walker" to fields(
                "Placed Walking Watching Charging Recovering Stunned" to "kind state x y vx facing",
                "Walking Watching Charging Recovering Stunned" to "sprite animFrame animDelay",
                "Watching" to "watchTime", "Charging" to "chargeSpeed", "Stunned" to "stunTime",
            ),
            "blower" to fields(
                "Placed Blowing Stunned" to "kind state x y",
                "Blowing Stunned" to "sprite",
                "Blowing" to "animFrame animDelay", "Stunned" to "stunTime",
            ),
            "tree" to fields(
                "Placed Swaying Resting Bare" to "kind state x y nextDrop",
                "Swaying Resting Bare" to "sprite",
                "Swaying Resting" to "animFrame animDelay", "Resting" to "restTime",
            ),
        )

        /** The fields of a bonus item by state. */
        val BONUS_FIELDS = fields("Falling Lying" to "kind state x y sprite", "Falling" to "vx vy", "Lying" to "lyingTime")

        /** The fields a test pilot may set between frames (the I lines). */
        val INJECTABLE = Regex("""copter\.\d\.(x|y|pixelX|pixelY|vx|vy|landedPad)|game\.(energy|lives)""")
    }
}
