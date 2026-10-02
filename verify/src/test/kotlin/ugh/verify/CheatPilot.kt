package ugh.verify

import ugh.core.game.Game
import ugh.oracle.OriginalUgh
import kotlin.random.Random

/**
 * Coverage helper for the passenger logic: between frames it writes the copter of player 1 straight into
 * the original's memory - landed on the pad where a passenger waits, on the pad a carried passenger wants to
 * go to, on the water surface next to a swimmer, or flying through a waiting passenger - and sometimes
 * presses fire (drops a hanging passenger). Oracle and port see the same state, so the differential test
 * stays exact; only the flying is skipped.
 */
class CheatPilot(private val ugh: OriginalUgh, seed: Long) {
    private val rnd = Random(seed)
    private var wait = 0
    private var fireFrames = 0
    val actions = sortedMapOf<String, Int>()

    fun attach() = ugh.onGameFrame { step() }

    /** Acts only at frame boundaries of a frame-level differential test (so both sides see the changes). */
    fun attachTo(diff: FrameDiff) { diff.beforeCopy += { step() } }

    /** Acts at the boundaries of a lockstep test: writes go to both states, keys to both sides. */
    fun attachTo(lockstep: Lockstep) { this.lockstep = lockstep; lockstep.atBoundary += { step() } }

    private var lockstep: Lockstep? = null

    private fun keys(vararg codes: Int) { val l = lockstep; if (l != null) codes.forEach(l::key) else ugh.machine.scancodes(*codes) }

    private fun w(off: Int, v: Int) { val l = lockstep; if (l != null) l.poke16(off, v) else ugh.machine.write16(OriginalUgh.SEG_DGROUP, off, v) }
    private fun r(off: Int) = ugh.dgroup16(off)
    private fun rs(off: Int) = ugh.dgroupS16(off)

    private fun inPlay() = rs(0x27a8) > 0x100 && rs(0x27aa) >= 0

    private var frame = 0
    private var floating = false
    private var idle = 0
    private var spaceFrames = 0

    /** While false, only keys still held are released (screens outside the level play are driven by the test). */
    var enabled = true

    private fun step() {
        if (fireFrames > 0 && --fireFrames == 0) keys(0xe0, 0x9d)
        if (spaceFrames > 0 && --spaceFrames == 0) keys(0xb9)
        if (!enabled) return
        if (!inPlay()) {
            // level caption / game over screens wait for a key (the game looks for a change of the last scancode)
            if (++idle % 150 == 0 && spaceFrames == 0) { keys(0x39); spaceFrames = 3; count("space") }
            return
        }
        idle = 0
        if (++frame % 50 == 0) { // keep the game going: energy and lives
            w(Game.V_ENERGY, 0x4000)
            if (ugh.dgroup8(0x263c) < 3) w(0x263c, (r(0x263c) and 0xff00) or 3)
        }
        // a walker charging at the landed copter: sometimes take off before it hits (the walker recovers)
        val charging = (0 until 5).map { it * 2 }.takeWhile { r(0x2cad + it) != 0xffff }.any { r(0x2cf3 + it) == 0x2748 }
        if (charging && rs(Game.P_LANDED) >= 0 && rnd.nextInt(10) < 3) {
            place(rs(Game.P_XF), rs(Game.P_YF) - 0x300, landed = -1)
            wait = 30
            count("escapeWalker")
            return
        }
        if (--wait > 0) {
            // never let the unpiloted copter fall for long (floating on the water is fine)
            if (rs(Game.P_LANDED) < 0 && !floating && wait > 12) wait = 12
            return
        }
        wait = 10 + rnd.nextInt(60)
        floating = false

        val carrying = r(0x27fc) != 0
        val passengers = (0 until 16).map { it * 2 }.takeWhile { r(0x29ad + it) != 0xffff }
        if (carrying && rnd.nextInt(10) < 8) {
            val p = passengers.firstOrNull { r(0x2a0d + it) == 0x1a42 && r(0x2a2d + it) == 0 }
            if (p != null) { landOn(r(0x2a6d + p)); count("deliver"); wait = 40 + rnd.nextInt(80); return }
        }
        // a passenger hanging below the copter: carry it over an enemy and drop it there
        val hanging = passengers.firstOrNull { r(0x2a0d + it) == 0x1c6b && r(0x2a2d + it) == 0 }
        val targets = (0 until 5).map { it * 2 }.takeWhile { r(0x2cad + it) != 0xffff }.filter { r(0x2d43 + it) != 0xffff }
        if (hanging != null && targets.isNotEmpty() && rnd.nextInt(10) < 7) {
            val o = targets[rnd.nextInt(targets.size)]
            // lead a moving target: the passenger needs ~20 frames to fall to it
            val lead = rs(0x2ce9 + o) * (10 + rnd.nextInt(20))
            place(rs(0x2cc1 + o) + lead - 0x200 + rnd.nextInt(0x100), rs(0x2ccb + o) - 0x700 - rnd.nextInt(0x200), landed = -1)
            keys(0xe0, 0x1d); fireFrames = 3
            wait = 30 + rnd.nextInt(40)
            count("dropOnEnemy")
            return
        }
        // state injection: make the standing passenger fall right onto a flying enemy
        val flyer = targets.firstOrNull { r(0x2cad + it) == 0x7630 && r(0x2cf3 + it) == 0x2493 }
        val standingAny = passengers.firstOrNull { r(0x29ad + it) == 0x78dc && r(0x2a0d + it) in setOf(0x1c27, 0x1c6b) }
        if (flyer != null && standingAny != null && rnd.nextInt(10) < 5) {
            if (r(0x2a0d + standingAny) == 0x1c6b) { w(0x27fc + r(0x2a2d + standingAny), 0); w(0x2804 + r(0x2a2d + standingAny), 0) }
            w(0x2a0d + standingAny, 0x1cee)
            w(0x2aed + standingAny, rs(0x2cc1 + flyer) + rs(0x2ce9 + flyer) * 4 - 0x80)
            w(0x2b0d + standingAny, rs(0x2ccb + flyer) - 0x180)
            w(0x2acd + standingAny, 0)
            w(0x2c6d + standingAny, 4)
            wait = 20
            count("injectHit")
            return
        }
        // grab a standing passenger
        val standing = passengers.firstOrNull { r(0x2a0d + it) == 0x1c27 }
        if (standing != null && !carrying && rnd.nextInt(10) < 4) {
            place(rs(0x2aed + standing) - 0x100, rs(0x2b0d + standing) - 0x200, landed = -1)
            wait = 5
            count("grab")
            return
        }
        if (rs(Game.P_LANDED) < 0 && rnd.nextInt(3) > 0) { landOn(rnd.nextInt(padCount())); count("land"); return }
        when (rnd.nextInt(10)) {
            in 0..5 -> if (carrying) {
                // deliver: the target pad of the passenger riding with player 1
                val p = passengers.firstOrNull { r(0x2a0d + it) == 0x1a42 && r(0x2a2d + it) == 0 }
                if (p != null) { landOn(r(0x2a6d + p)); count("deliver") }
            } else {
                val waiting = passengers.filter { r(0x2a0d + it) in WAITING_STATES }
                if (waiting.isNotEmpty()) {
                    landOn(r(0x2a4d + waiting[rnd.nextInt(waiting.size)]))
                    wait = 120 + rnd.nextInt(120) // give it time to walk over and board
                    count("pickup")
                }
            }
            6 -> { // fly through a waiting passenger (knocks it into the water)
                val waiting = passengers.filter { r(0x2a0d + it) in WAITING_STATES }
                if (waiting.isNotEmpty()) {
                    val p = waiting[rnd.nextInt(waiting.size)]
                    place(rs(0x2aed + p) - 0x100, rs(0x2b0d + p) - 0x200, landed = -1)
                    count("flyThrough")
                }
            }
            7 -> { // float on the water surface next to a swimmer, or anywhere on it
                val swimmer = passengers.firstOrNull { r(0x2a0d + it) in SWIM_STATES }
                val water = rs(Game.V_WATER_ROW)
                val x = if (swimmer != null) rs(0x2aed + swimmer) else rnd.nextInt(0x2000)
                if (water < 0xc0) {
                    place(x, ((water shl 5) - 0x240), landed = -1)
                    floating = true
                    wait = 100 + rnd.nextInt(300)
                    count("water")
                }
            }
            8 -> { keys(0xe0, 0x1d); fireFrames = 5; count("fire") }
            else -> { place(rnd.nextInt(0x2400), rnd.nextInt(0x1000), landed = -1); count("teleport") }
        }
    }

    private fun count(what: String) { actions.merge(what, 1, Int::plus) }

    private fun padCount() = (0 until 10).takeWhile { rs(Game.PAD_LEFT + 2 * it) >= 0 }.size.coerceAtLeast(1)

    /** Copter of player 1 landed on pad [pad]. */
    private fun landOn(pad: Int) {
        val left = rs(Game.PAD_LEFT + 2 * pad)
        val right = rs(Game.PAD_RIGHT + 2 * pad)
        val y = rs(Game.PAD_Y + 2 * pad)
        val x = (left + right) / 2 - 0x10
        place(x shl 5, (y - 0x14) shl 5, landed = pad)
    }

    /**
     * Within the range the copter physics keeps it in (113b:1095: x -0x200..0x2600, y -0x260..0x17e0); further
     * up the collision probe would read the draw page above the background page.
     */
    private fun place(x: Int, y: Int, landed: Int) {
        val xf = x.coerceIn(-0x200, 0x2600)
        val yf = y.coerceIn(-0x260, 0x17e0)
        w(Game.P_XF, xf); w(Game.P_X, xf shr 5)
        w(Game.P_YF, yf); w(Game.P_Y, yf shr 5)
        w(Game.P_VX, 0); w(Game.P_VY, 0)
        w(Game.P_LANDED, landed)
    }

    companion object {
        private val WAITING_STATES = setOf(0x15d7, 0x172a, 0x180a, 0x18e6)
        private val SWIM_STATES = setOf(0x1f43, 0x1fe2, 0x2068, 0x20c1)
    }
}
