package ugh.core.game

/*
 * Level sequence of a game - port of 113b:0c61 .. 0fe7 (after the password check and the opening cut scene):
 * new game, then level attempts until the game is over or all levels are done.
 */

enum class GameEnd { GAME_OVER, ALL_LEVELS_DONE }

/** 113b:3961 - new game: 3 lives, multiplier 1, score 0. */
fun Game.newGame() {
    setD8(0x263c, 3)
    setD8(0x263d, 1)
    setD(0x261e, 0)
    setD(0x2620, 0)
}

/**
 * 113b:0c7d .. 0fa4 with the retrace waits: plays frames until the level ends (fade-out finished).
 * The pause (P) waits like the original: P released, then any key pressed and released.
 */
fun Game.playLevel(r: Regs = Regs()) {
    while (true) {
        val cx = d(Game.V_FADE)
        if (cx < 0) return
        if (cx <= 0x100) {
            fadeStep(cx)
            setD(Game.V_FADE, cx + d(Game.V_FADE_STEP))
        } else {
            vsync()
        }
        if (frameBody(r) == FrameResult.PAUSE) {
            // 113b:0d85 .. 0d98: busy loops on the last scancode, no retrace waits
            do { host.idle(); readScancode(r) } while (r.ax and 0xff != 0x99)
            do { host.idle(); readScancode(r) } while (r.ax and 0x80 != 0)
            do { host.idle(); readScancode(r) } while (r.ax and 0x80 == 0)
            frameAfterKeys(r.ax and 0xff, r)
        }
    }
}

/** 113b:0c61 .. 0fe7 - new game and the level loop from the level in DGROUP:261c. */
fun Game.playGame(r: Regs = Regs()): GameEnd {
    newGame()
    blackPalette()
    while (true) {
        levelSetup(r)
        musicStart(0x4444, 0, 0xa5)
        playLevel(r)
        soundControl(0x11)
        if (d8(0x27cf) and 0x80 != 0) {
            // level completed
            val next = u(0x261c) + 1
            setD(0x261c, next)
            if (next >= u(0x2624 + u(0x2636) - 2)) return GameEnd.ALL_LEVELS_DONE
        } else {
            // life lost
            val lives = (d8(0x263c) - 1) and 0xff
            setD8(0x263c, lives)
            if (lives == 0 || lives and 0x80 != 0) return GameEnd.GAME_OVER
            setD8(0x263d, 1)
        }
    }
}
