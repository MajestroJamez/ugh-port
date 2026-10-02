package ugh.verify.replay

import ugh.core.game.Game

/**
 * Projection of the original's memory (port or oracle - both use the same address space) onto a
 * semantic game state: named, flat fields in the original's units (positions in 1/32 px, speeds in
 * 1/64 of that per frame, state machines by name). This is what a reimplementation outside the DOS
 * address space (the C++ core) has to reproduce tick by tick.
 *
 * The state is a sorted map "group.index.field" -> value; values are decimal integers or names.
 */
class StateProjection(private val read16: (seg: Int, off: Int) -> Int) {

    private fun u(off: Int) = read16(Game.DGROUP, off) and 0xffff
    private fun s(off: Int) = Game.s16(u(off))
    private fun b(off: Int) = read16(Game.DGROUP, off) and 0xff
    private fun cs(off: Int) = read16(Game.CODE, off) and 0xffff

    /**
     * [phase] is where the game waits for this frame (see [phaseOfPort]); the level lists (pads, passengers,
     * objects, bonus items) are only meaningful once a level is loaded, so they are left out before.
     */
    fun project(phase: String): Map<String, String> {
        val m = sortedMapOf<String, String>()
        fun put(k: String, v: Any) { m[k] = v.toString() }
        put("game.phase", phase)

        // ---------------------------------------------------------------- game
        val players = u(0x2634)
        put("game.level", u(0x261c))
        put("game.players", players)
        put("game.difficulty", u(0x2638))
        put("game.lives", b(0x263c))
        put("game.multiplier", b(0x263d))
        put("game.score", (u(0x2620).toLong() shl 16) or u(0x261e).toLong())
        put("game.energy", s(Game.V_ENERGY))
        put("game.fade", s(Game.V_FADE))
        put("game.fadeStep", s(Game.V_FADE_STEP))
        put("game.levelDone", if (b(0x27cf) and 0x80 != 0) 1 else 0)
        put("game.wind", b(Game.V_WIND))
        put("game.waterRow", s(Game.V_WATER_ROW))
        put("game.rng", "%04x%04x%04x%04x".format(cs(0x4efd), cs(0x4efb), cs(0x4ef9), cs(0x4ef7)))

        if (phase !in LEVEL_PHASES) return m

        // ---------------------------------------------------------------- copters
        for (p in 0 until players.coerceIn(1, 2)) {
            val bx = 2 * p
            val c = "copter.$p."
            put(c + "xf", s(Game.P_XF + bx))
            put(c + "yf", s(Game.P_YF + bx))
            put(c + "vx", s(Game.P_VX + bx))
            put(c + "vy", s(Game.P_VY + bx))
            put(c + "landedPad", s(Game.P_LANDED + bx))
            put(c + "effort", s(Game.P_EFFORT + bx))
            put(c + "impact", s(Game.P_IMPACT + bx))
            put(c + "carrying", hexOrNone(u(0x27fc + bx), none = 0))
            put(c + "targetPad", s(0x2804 + bx))
            put(c + "fare", s(0x2808 + bx))
            put(c + "fareMin", s(0x280c + bx))
            put(c + "keys", buildString {
                if (u(Game.KEY_UP + bx) != 0) append('U')
                if (u(Game.KEY_DOWN + bx) != 0) append('D')
                if (u(Game.KEY_LEFT + bx) != 0) append('L')
                if (u(Game.KEY_RIGHT + bx) != 0) append('R')
                if (u(0x279c + bx) != 0) append('F')
                if (isEmpty()) append('-')
            })
        }

        // ---------------------------------------------------------------- pads (level list A)
        for (i in 0 until 10) {
            val si = 2 * i
            if (s(Game.PAD_LEFT + si) < 0) break
            val c = "pad.$i."
            put(c + "left", s(Game.PAD_LEFT + si))
            put(c + "right", s(Game.PAD_RIGHT + si))
            put(c + "y", s(Game.PAD_Y + si))
            put(c + "number", s(0x2985 + si))
            put(c + "waiting", s(0x2999 + si))
        }

        // ---------------------------------------------------------------- passengers (level list B)
        for (i in 0 until 16) {
            val si = 2 * i
            val desc = u(0x29ad + si)
            if (desc == 0xffff) break
            val c = "passenger.$i."
            put(c + "kind", hex(desc))
            put(c + "state", PASSENGER_STATES[u(0x2a0d + si)] ?: "?" + hex(u(0x2a0d + si)))
            put(c + "xf", s(0x2aed + si))
            put(c + "yf", s(0x2b0d + si))
            put(c + "vy", s(0x2c6d + si))
            put(c + "timer", s(0x2acd + si))
            put(c + "counter", s(0x2a2d + si))
            put(c + "pickupPad", s(0x2a4d + si))
            put(c + "targetPad", s(0x2a6d + si))
            put(c + "bonusTimer", s(0x2c8d + si))
            put(c + "sprite", hexOrNone(u(0x2bed + si)))
            put(c + "bubble", hexOrNone(u(0x2c2d + si)))
        }

        // ---------------------------------------------------------------- objects (level list C)
        for (i in 0 until 5) {
            val si = 2 * i
            val desc = u(0x2cad + si)
            if (desc == 0xffff) break
            val c = "object.$i."
            put(c + "kind", OBJECT_KINDS[desc] ?: hex(desc))
            put(c + "state", OBJECT_STATES[u(0x2cf3 + si)] ?: "?" + hex(u(0x2cf3 + si)))
            put(c + "xf", s(0x2cc1 + si))
            put(c + "yf", s(0x2ccb + si))
            put(c + "vx", s(0x2ce9 + si))
            put(c + "timer", s(0x2cfd + si))
            put(c + "facing", s(0x2d57 + si))
            put(c + "sprite", hexOrNone(u(0x2d43 + si)))
        }

        // ---------------------------------------------------------------- bonus items (12 slots)
        for (i in 0 until 12) {
            val si = 2 * i
            val sprite = u(0x2d9b + si)
            if (sprite == 0xffff) continue
            val c = "bonus.$i."
            put(c + "kind", hex(u(0x2d6b + si)))
            put(c + "state", BONUS_STATES[u(0x2d83 + si)] ?: "?" + hex(u(0x2d83 + si)))
            put(c + "xf", s(0x2dcb + si))
            put(c + "yf", s(0x2de3 + si))
            put(c + "vx", s(0x2e5b + si))
            put(c + "vy", s(0x2e73 + si))
            put(c + "sprite", hex(sprite))
        }
        return m
    }

    companion object {
        /** Phases in which a level is loaded. */
        val LEVEL_PHASES = setOf("caption", "play")

        /** Port functions that wait for a retrace, innermost first wins: function name -> phase. */
        private val PHASE_FUNCTIONS = mapOf(
            "playLevel" to "play", "levelCaption" to "caption", "levelSetup" to "setup",
            "afterGame" to "afterGame", "highScoreEntry" to "highScores", "playGame" to "betweenLevels",
        )

        /**
         * The phase of the port's current retrace wait, from its call stack (call from inside Host.frame):
         * "play" (level play incl. its fades), "caption" (level caption and its fades), "setup", ...
         */
        fun phaseOfPort(): String = StackWalker.getInstance().walk { frames ->
            frames.map { PHASE_FUNCTIONS[it.methodName] }.filter { it != null }.findFirst().orElse(null)
        } ?: "other"

        private fun hex(v: Int) = "0x%04x".format(v)
        private fun hexOrNone(v: Int, none: Int = 0xffff) = if (v == none) "none" else hex(v)

        val OBJECT_KINDS = mapOf(0x7630 to "flyer", 0x766c to "walker", 0x76a8 to "blower", 0x76e4 to "tree")

        /** Passenger state handlers of the original (Passengers.kt) by address. */
        val PASSENGER_STATES = mapOf(
            0x149c to "NextStop", 0x1509 to "Arriving", 0x153b to "Appear", 0x1582 to "Appearing",
            0x15b4 to "StartWaiting", 0x15d7 to "Waiting", 0x16f6 to "StartCalling", 0x172a to "Calling",
            0x17e6 to "StartImpatient", 0x180a to "Impatient", 0x18c8 to "StartBoarding", 0x18e6 to "Boarding",
            0x19e0 to "Board", 0x19fb to "BoardKeepDescriptor", 0x1a42 to "Riding", 0x1a7e to "Paid",
            0x1b29 to "WalkingAway", 0x1bbe to "StartEntering", 0x1bd6 to "Entering", 0x1c0e to "Gone",
            0x2195 to "Idle", 0x1c0f to "StartStanding", 0x1c27 to "Standing", 0x1c48 to "Grabbed",
            0x1c6b to "Hanging", 0x1c81 to "Dropped", 0x1cee to "Falling", 0x1da8 to "StartSplash",
            0x1dd5 to "Splash", 0x1de2 to "SplashAdvance", 0x1e9c to "StartSinking", 0x1ec0 to "Sinking",
            0x1f24 to "StartSwimming", 0x1f43 to "Swimming", 0x1fe2 to "SwimCalling", 0x2068 to "SwimWaving",
            0x20c1 to "SwimBoarding",
        )

        /** Object state handlers of the original (Objects.kt) by address. */
        val OBJECT_STATES = mapOf(
            0x2379 to "FlyerInit", 0x239f to "FlyerWait", 0x23b0 to "FlyerScreech", 0x23d9 to "FlyerWait2",
            0x23ea to "FlyerStart", 0x2493 to "Flying", 0x252b to "FlyerHit", 0x255e to "FlyerFalling",
            0x25b1 to "WalkerInit", 0x25c9 to "Walking", 0x2667 to "StartWatching", 0x2681 to "Watching",
            0x272e to "StartCharging", 0x2748 to "Charging", 0x2830 to "StartRecovering",
            0x288f to "StartRecovering2", 0x2844 to "Recovering", 0x28a3 to "Recovering2",
            0x28ee to "WalkerStunned", 0x2914 to "Stunned", 0x295b to "BlowerInit", 0x2973 to "Blowing",
            0x2a53 to "BlowerStunned", 0x2a76 to "BlowerWait", 0x2a87 to "TreeInit", 0x2ab5 to "Tree",
            0x2b0c to "TreeCatch", 0x2b58 to "TreeWait", 0x2b7e to "Inactive", 0x1c0e to "Gone",
        )

        /** Bonus item state handlers of the original (Bonuses.kt) by address. */
        val BONUS_STATES = mapOf(0x2b96 to "Spawn", 0x2be4 to "Falling", 0x2c97 to "Landed", 0x2ca9 to "Lying")
    }
}
