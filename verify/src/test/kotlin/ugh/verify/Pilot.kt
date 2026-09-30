package ugh.verify

import ugh.oracle.OriginalUgh
import kotlin.random.Random

/**
 * Deterministic pseudo-random pilot: holds random combinations of the player's controls for random
 * durations, so that differential tests see flying, climbing, diving, bumping into walls and landing.
 */
class Pilot(private val ugh: OriginalUgh, seed: Long, private val player: Int = 0) {
    private val rnd = Random(seed)
    private val held = BooleanArray(4)
    private var framesLeft = 0

    // make codes: up, down, left, right (player 1: grey arrows with E0 prefix, player 2: W Z A S)
    private val codes = if (player == 0) intArrayOf(0x48, 0x50, 0x4b, 0x4d) else intArrayOf(0x11, 0x2c, 0x1e, 0x1f)
    private val extended = player == 0

    fun attach() = ugh.onGameFrame { step() }

    /** Presses its keys at the boundaries of a lockstep test (delivered to both sides). */
    fun attachTo(lockstep: Lockstep) { this.lockstep = lockstep; lockstep.atBoundary += { step() } }

    private var lockstep: Lockstep? = null

    private fun keys(vararg codes: Int) { val l = lockstep; if (l != null) codes.forEach(l::key) else ugh.machine.scancodes(*codes) }

    private fun step() {
        if (--framesLeft > 0) return
        framesLeft = 1 + rnd.nextInt(40)
        val want = BooleanArray(4)
        want[0] = rnd.nextInt(100) < 55          // pedal more often than not
        want[1] = !want[0] && rnd.nextInt(100) < 15
        when (rnd.nextInt(3)) { 1 -> want[2] = true; 2 -> want[3] = true }
        for (k in 0..3) if (want[k] != held[k]) {
            val code = if (want[k]) codes[k] else codes[k] or 0x80
            if (extended) keys(0xe0, code) else keys(code)
            held[k] = want[k]
        }
    }
}
