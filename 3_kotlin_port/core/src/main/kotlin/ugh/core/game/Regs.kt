package ugh.core.game

/**
 * Register file for routines that pass values in registers (the state machines jump between handlers
 * with JMP [SI+n] and hand over SI/DI/AX). Values are unsigned 16-bit.
 */
class Regs(
    var ax: Int = 0, var bx: Int = 0, var cx: Int = 0, var dx: Int = 0,
    var si: Int = 0, var di: Int = 0, var bp: Int = 0,
) {
    override fun toString() = "AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x".format(ax, bx, cx, dx, si, di, bp)
}
