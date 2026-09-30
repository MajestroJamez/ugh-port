package ugh.core.game

import ugh.core.game.Game.Companion.CODE
import ugh.core.game.Game.Companion.s16

/*
 * One frame of level play - port of the game loop body 113b:0c7d .. 0fa4 (without the wait for the
 * vertical retrace, which is the caller's business).
 */

enum class FrameResult {
    /** Normal frame; the loop continues. */
    CONTINUE,
    /** DGROUP:27a8 went negative: the level ended (fade-out finished), 113b:0fa7 follows. */
    LEVEL_END,
    /** The P key was pressed: the original waits here for P to be released and pressed again. */
    PAUSE,
}

/** 113b:44f1 - reads the last scancode from the keyboard handler: AL = scancode, AH = changed bits. */
fun Game.readScancode(r: Regs) {
    setD8(0x2645, d8(0x2643))
    val al = mem.read8(CODE, 0x4509)
    setD8(0x2643, al)
    val ah = al xor d8(0x2645)
    setD8(0x2646, ah)
    r.ax = (ah shl 8) or al
}

/**
 * 113b:0fe8 - key check of the frame; joystick players (DGROUP:263f/2641 >= 0) get their controls from the
 * joystick position. The port supports keyboard control only: 113b:51ec (joystick polling) is not called.
 * Returns AX = scancode / changed like 44f1.
 */
fun Game.frameKeys(r: Regs) {
    readScancode(r)
    var bx = 0
    while (true) {
        var si = u(0x263f + bx)
        si = (si shl 1) and 0xffff
        if (s16(si) >= 0) {
            for (k in intArrayOf(0x278c, 0x2790, 0x2794, 0x2798, 0x279c)) setD(k + bx, 0)
            var ax = u(0x3315 + si)
            if (s16(ax) <= d(0x3331 + si)) addD(0x2794 + bx, -1)
            else if (s16(ax) >= d(0x3335 + si)) addD(0x2798 + bx, -1)
            ax = u(0x3319 + si)
            if (s16(ax) <= d(0x3339 + si)) addD(0x278c + bx, -1)
            else if (s16(ax) >= d(0x333d + si)) addD(0x2790 + bx, -1)
            if (u(0x331d + si) != 0) addD(0x279c + bx, -1)
        }
        bx += 2
        if (bx != u(0x2634)) break
    }
    r.ax = (d8(0x2646) shl 8) or d8(0x2643)
}

/** The vsync/fade part of 113b:0c7d; returns false when the level ended. */
fun Game.frameFade(): Boolean {
    val cx = d(Game.V_FADE)
    if (cx < 0) return false
    if (cx <= 0x100) {
        fadePalette(cx)
        setD(Game.V_FADE, cx + d(Game.V_FADE_STEP))
    }
    return true
}

/** 113b:0ca5 .. 0fa4 - everything of a frame after the retrace wait. */
fun Game.frameBody(r: Regs = Regs()): FrameResult {
    updateStatusBar()
    run { var bx = 0; do { eraseCopter(bx); bx += 2 } while (bx != u(0x2636)) }

    // erase passengers and their bubbles
    var si = 0
    while (u(0x29ad + si) != 0xffff) {
        val cur = u(0x2bed + si)
        val old = u(0x2c0d + si)
        setD(0x2c0d + si, cur)
        if (old != 0xffff && d(0x2b8d + si) < 0xc0) {
            eraseSprite(old, u(0x2b4d + si), u(0x2b8d + si))
            val curBubble = u(0x2c2d + si)
            val oldBubble = u(0x2c4d + si)
            setD(0x2c4d + si, curBubble)
            if (oldBubble != 0xffff) eraseSprite(oldBubble, (u(0x2b4d + si) + 0xb) and 0xffff, (u(0x2b8d + si) - 0xd) and 0xffff)
        }
        si += 2
    }
    // erase objects
    si = 0
    while (u(0x2cad + si) != 0xffff) {
        val cur = u(0x2d43 + si)
        val old = u(0x2d4d + si)
        setD(0x2d4d + si, cur)
        if (old != 0xffff) eraseSprite(old, u(0x2d11 + si), u(0x2d25 + si))
        si += 2
    }
    // erase bonus items
    si = 0x16
    while (si >= 0) {
        val cur = u(0x2d9b + si)
        val old = u(0x2db3 + si)
        setD(0x2db3 + si, cur)
        if (old != 0xffff) eraseSprite(old, u(0x2e13 + si), u(0x2e43 + si))
        si -= 2
    }
    if (d8(Game.V_WIND) != 0) eraseRain()
    updateWater()

    frameKeys(r)
    val key = r.ax and 0xff
    if (key == 0x19) return FrameResult.PAUSE
    return frameAfterKeys(key, r)
}

/**
 * Second half of the frame (113b:0d9a ..). After a PAUSE the caller waits like the original (P released,
 * then any key pressed and released) and calls this with the scancode of the key that ended the pause.
 */
fun Game.frameAfterKeys(key: Int, r: Regs = Regs()): FrameResult {
    var si: Int
    if (key == 0x01) {
        setD8(0x263c, 0)
        if (d(Game.V_FADE_STEP) >= 0) { setD(Game.V_FADE_STEP, 0xfffe); addD(Game.V_FADE, -2) }
    }

    // logic
    if (d(Game.V_FADE) > 0xc0 || d(Game.V_FADE_STEP) <= 0) {
        var bx = 0
        do { copterUpdate(bx); bx += 2 } while (bx == u(0x2634))
    }
    passengersUpdate(r)
    objectsUpdate(r)
    bonusesUpdate(r)

    // draw passengers
    si = 0
    while (u(0x29ad + si) != 0xffff) {
        val sprite = u(0x2bed + si)
        if (sprite != 0xffff) {
            setD(0x2b4d + si, u(0x2b2d + si))
            setD(0x2b8d + si, u(0x2b6d + si))
            val x = s16(u(0x2aed + si)) shr 5
            val y = s16(u(0x2b0d + si)) shr 5
            setD(0x2b2d + si, x); setD(0x2b6d + si, y)
            if (y < 0xc0 && y > -0xb) drawSprite(sprite, x and 0xffff, y and 0xffff)
        }
        si += 2
    }
    // draw objects: trees and blowers behind the copters, the others in front
    drawObjects(background = true)
    run { var bx = 0; do { drawCopter(bx); bx += 2 } while (bx != u(0x2636)) }
    drawObjects(background = false)
    // draw bonus items
    si = 0x16
    while (si >= 0) {
        val sprite = u(0x2d9b + si)
        if (sprite != 0xffff) {
            setD(0x2e13 + si, u(0x2dfb + si))
            setD(0x2e43 + si, u(0x2e2b + si))
            val x = s16(u(0x2dcb + si)) shr 5
            val y = s16(u(0x2de3 + si)) shr 5
            setD(0x2dfb + si, x); setD(0x2e2b + si, y)
            if (y < 0xc0) drawSprite(sprite, x and 0xffff, y and 0xffff)
        }
        si -= 2
    }
    // draw bubbles
    si = 0
    while (u(0x29ad + si) != 0xffff) {
        if (u(0x2bed + si) != 0xffff && u(0x2c2d + si) != 0xffff) {
            drawSprite(u(0x2c2d + si), (u(0x2b2d + si) + 0xb) and 0xffff, (u(0x2b6d + si) - 0xd) and 0xffff)
        }
        si += 2
    }
    if (d8(Game.V_WIND) != 0) { moveRain(); drawRain() }
    drawWaterSurface()
    flipPages()
    return FrameResult.CONTINUE
}

private fun Game.drawObjects(background: Boolean) {
    var si = 0
    while (true) {
        val type = u(0x2cad + si)
        if (type == 0xffff) return
        val isBackground = type == 0x76e4 || type == 0x76a8
        val sprite = u(0x2d43 + si)
        if (isBackground == background && sprite != 0xffff) {
            setD(0x2d11 + si, u(0x2d07 + si))
            setD(0x2d25 + si, u(0x2d1b + si))
            val x = s16(u(0x2cc1 + si)) shr 5
            val y = s16(u(0x2ccb + si)) shr 5
            setD(0x2d07 + si, x); setD(0x2d1b + si, y)
            if (y < 0xc0) drawSprite(sprite, x and 0xffff, y and 0xffff)
        }
        si += 2
    }
}
