package ugh.oracle

import ugh.extractor.IndexedImage
import ugh.extractor.Palette

/**
 * The original UGH.EXE running in [Machine], with UGH specific knowledge:
 * a "game frame" is one call of the vsync wait 113b:44c3 (the game logic runs once per call).
 */
class OriginalUgh(exe: ByteArray, disk: VirtualDisk = VirtualDisk()) {
    val machine = Machine(exe, disk)

    /** Number of calls of the vsync wait so far. */
    var gameFrame = 0L
        private set

    private val frameHooks = mutableListOf<(Long) -> Unit>()

    init {
        // the two polling loops of 113b:44c3 / 44d2
        machine.pollSites += lin(SEG_GAME, 0x44c9)
        machine.pollSites += lin(SEG_GAME, 0x44d5)
        machine.onExecute(SEG_GAME, 0x44c3) {
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
            while (gameFrame < target && !machine.exited) machine.run(50_000_000L)
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
            while (!hit && gameFrame < limit && !machine.exited) machine.run(10_000_000L)
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
    fun startGame(menuKeys: List<Int> = emptyList(), log: (String) -> Unit = {}): Boolean {
        if (!runUntil(SEG_GAME, 0x00d8)) return false
        log("menu at frame $gameFrame")
        runGameFrames(10)
        for (k in menuKeys) { tap(k); runGameFrames(10) }
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
    }
}
