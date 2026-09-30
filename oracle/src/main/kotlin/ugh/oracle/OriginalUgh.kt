package ugh.oracle

import ugh.extractor.IndexedImage
import ugh.extractor.Palette

/**
 * The original UGH.EXE running in [Machine], with UGH specific knowledge:
 * a "game frame" is one wait for the start of a vertical retrace, 113b:44c6 (113b:44c3 falls into it; the
 * password entry and a few other loops call 44c6 directly). The game logic runs once per frame.
 */
class OriginalUgh(exe: ByteArray, disk: VirtualDisk = VirtualDisk()) {
    val machine = Machine(exe, disk)

    /** Number of retrace waits so far. */
    var gameFrame = 0L
        private set

    private val frameHooks = mutableListOf<(Long) -> Unit>()

    init {
        // the two polling loops of 113b:44c3 / 44d2
        machine.pollSites += lin(SEG_GAME, 0x44c9)
        machine.pollSites += lin(SEG_GAME, 0x44d5)
        machine.onExecute(SEG_GAME, 0x44c6) {
            gameFrame++
            for (h in frameHooks) h(gameFrame)
        }
    }

    /** Called at the start of every vsync wait, i.e. between two frames of game logic. */
    fun onGameFrame(hook: (Long) -> Unit) { frameHooks += hook }

    /** Runs until [n] more game frames have started (or the program exits). */
    fun runGameFrames(n: Long) {
        val target = gameFrame + n
        val hook: (Long) -> Unit = { if (it >= target) machine.stop() }
        frameHooks += hook
        try {
            val vgaLimit = machine.vgaFrame + 4 * n + 1000   // safety net if the program stops waiting for retraces
            while (gameFrame < target && !machine.exited && machine.vgaFrame < vgaLimit) machine.run(5_000_000L)
        } finally {
            frameHooks -= hook
        }
    }

    /** Runs until execution reaches seg:off (at most [maxFrames] game frames); returns true if reached. */
    fun runUntil(seg: Int, off: Int, maxFrames: Long = 100_000): Boolean {
        var reached = false
        val target = lin(seg, off)
        if (target !in watched) {
            watched += target
            machine.onExecute(seg, off) { if (target == waitingFor) { hit = true; machine.stop() } }
        }
        waitingFor = target
        hit = false
        val limit = gameFrame + maxFrames
        val hook: (Long) -> Unit = { if (it >= limit) machine.stop() }
        frameHooks += hook
        try {
            val vgaLimit = machine.vgaFrame + 4 * maxFrames + 1000
            while (!hit && gameFrame < limit && !machine.exited && machine.vgaFrame < vgaLimit) machine.run(5_000_000L)
        } finally {
            frameHooks -= hook
        }
        reached = hit
        waitingFor = -1
        return reached
    }

    /** Presses and releases a key (make code, 0xE0 prefix passed separately if needed). */
    fun tap(make: Int, holdFrames: Long = 3) {
        machine.scancodes(make)
        runGameFrames(holdFrames)
        machine.scancodes(make or 0x80)
        runGameFrames(holdFrames)
    }

    /**
     * From power-on to the first frame of level play: waits for the main menu, applies [menuKeys]
     * (e.g. F3 for difficulty, F4 for team mode), presses F1, skips the cut scene and the level caption
     * with SPACE and stops at the first call of the frame renderer in the game loop (113b:0ca5).
     */
    /** Types letters, digits and spaces (as scancodes, one key per 2 frames). */
    fun type(text: String) {
        for (ch in text.uppercase()) {
            val code = when (ch) {
                in 'A'..'Z' -> LETTER_SCANCODES[ch - 'A']
                in '1'..'9' -> 0x02 + (ch - '1')
                '0' -> 0x0b
                ' ' -> 0x39
                '\n' -> 0x1c
                else -> error("cannot type '$ch'")
            }
            tap(code, holdFrames = 2)
        }
    }

    fun startGame(menuKeys: List<Int> = emptyList(), password: String? = null, log: (String) -> Unit = {}): Boolean {
        if (!runUntil(SEG_GAME, 0x00d8)) return false
        log("menu at frame $gameFrame")
        runGameFrames(10)
        for (k in menuKeys) { tap(k); runGameFrames(10) }
        if (password != null) { tap(0x3c); runGameFrames(10); type(password + "\n"); runGameFrames(10) }
        tap(0x3b)
        log("F1 done at frame $gameFrame")
        repeat(40) {
            if (runUntil(SEG_GAME, 0x0ca5, 100)) { log("level play at frame $gameFrame"); return true }
            tap(0x39)
            log("SPACE at frame $gameFrame")
        }
        return false
    }

    private val watched = HashSet<Int>()
    private var waitingFor = -1
    private var hit = false

    fun dgroup8(off: Int) = machine.read8(SEG_DGROUP, off)
    fun dgroup16(off: Int) = machine.read16(SEG_DGROUP, off)
    fun dgroupS16(off: Int) = dgroup16(off).toShort().toInt()

    /** Copy of [length] bytes of DGROUP starting at [off]. */
    fun dgroup(off: Int, length: Int) = ByteArray(length) { machine.read8(SEG_DGROUP, off + it).toByte() }

    /** Current screen as seen on the monitor. */
    fun screenshot(): Pair<IndexedImage, Palette> {
        val pal = ByteArray(768) { machine.vga.dac[it].toByte() }
        return IndexedImage(320, 200, machine.vga.renderIndexed()) to Palette(pal)
    }

    companion object {
        const val SEG_GAME = 0x113b
        const val SEG_DGROUP = 0x6c09
        fun lin(seg: Int, off: Int) = (seg shl 4) + off

        private val LETTER_SCANCODES = intArrayOf(
            0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
            0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c,
        )
    }
}
