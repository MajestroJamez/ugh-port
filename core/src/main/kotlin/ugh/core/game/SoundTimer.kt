package ugh.core.game

/*
 * Multi-rate timer scheduler over INT 8 / PIT channel 0 - port of segment 1a32. Up to 16 slots, each with a
 * 32-bit period in PIT clocks and a far callback; the PIT runs at the shortest period and every interrupt
 * subtracts the elapsed time from all slots. Variables in DGROUP (index SI = slot * 2):
 * 7a4e/7a6e period, 7a8e/7aae countdown, 7ace due flag, 7aee handle -> slot, 7b0e slot -> handle,
 * 7b2e + 2*SI callback, 7a4c last slot, 7b6e/7b70 elapsed per interrupt, 7b72/7b74 old INT 8,
 * 7b76/7b78 shortest period, 7b7a PIT divisor programmed.
 */

internal const val TIMER_SEG = 0x1a32
private const val SLOTS_LAST = 0x7a4c
private const val PERIOD_LO = 0x7a4e
private const val PERIOD_HI = 0x7a6e
private const val COUNT_LO = 0x7a8e
private const val COUNT_HI = 0x7aae
private const val DUE = 0x7ace
private const val HANDLE_SLOT = 0x7aee
private const val SLOT_HANDLE = 0x7b0e
private const val CALLBACK = 0x7b2e
private const val ELAPSED_LO = 0x7b6e
private const val ELAPSED_HI = 0x7b70
private const val OLD_INT8 = 0x7b72
private const val MIN_LO = 0x7b76
private const val MIN_HI = 0x7b78
private const val PIT_DIVISOR = 0x7b7a

/** Far address seg:off as one Int, for the callback dispatch. */
internal fun far(seg: Int, off: Int) = (seg shl 16) or off

/** 1a32:0008 - installs the scheduler (once): slot 0 calls the old INT 8 handler every 65536 PIT clocks. */
internal fun Game.timerInstall() {
    if (mem.read16(TIMER_SEG, 6) != 0) return
    mem.write16(TIMER_SEG, 6, 1)
    setD(OLD_INT8, mem.read16(0, 0x20)); setD(OLD_INT8 + 2, mem.read16(0, 0x22))   // INT 21h AH=35h
    setD(CALLBACK, 0x0099); setD(CALLBACK + 2, TIMER_SEG)
    setD(ELAPSED_LO, 0); setD(ELAPSED_HI, 1)
    setD(MIN_LO, 0); setD(MIN_HI, 1)
    setD(PERIOD_LO, 0); setD(PERIOD_HI, 1)
    setD(COUNT_LO, 0); setD(COUNT_HI, 1)
    for (si in 0x1e downTo 0 step 2) setD(HANDLE_SLOT + si, 0x8000)
    setD(HANDLE_SLOT, 0)
    setD(SLOT_HANDLE, 0)
    setD(SLOTS_LAST, 0)
    // INT 8 -> 1a32:00d2 (the vector itself is the host's business)
}

/** 1a32:00a0 - uninstalls the scheduler: old INT 8 back, PIT to 65536. */
internal fun Game.timerUninstall() {
    if (mem.read16(TIMER_SEG, 6) == 0) return
    mem.write16(TIMER_SEG, 6, 0)
    host.timerDivisor(0)
}

/** 32-bit compare helper: is hi:lo below cHi:cLo (unsigned)? */
private fun below(hi: Int, lo: Int, cHi: Int, cLo: Int) = hi < cHi || (hi == cHi && lo < cLo)

/** 1a32:01aa - adds a slot with callback [callback] (far) and period cx:bx; returns its handle. */
internal fun Game.timerAdd(callback: Int, bx: Int, cx: Int): Int {
    val si = u(SLOTS_LAST) + 2
    setD(PERIOD_LO + si, bx); setD(PERIOD_HI + si, cx)
    setD(COUNT_LO + si, bx); setD(COUNT_HI + si, cx)
    setD(CALLBACK + 2 * si, callback and 0xffff); setD(CALLBACK + 2 * si + 2, callback ushr 16)
    if (below(cx, bx, u(MIN_HI), u(MIN_LO))) { setD(MIN_LO, bx); setD(MIN_HI, cx) }
    var di = si
    while (u(HANDLE_SLOT + di) and 0x8000 == 0) di = (di - 2) and 0xffff
    setD(SLOT_HANDLE + si, di)
    setD(HANDLE_SLOT + di, si)
    setD(SLOTS_LAST, si)
    return di
}

/** Shortest period of all slots, starting from cx:bx (the '<=' takes the later slot as the original does). */
private fun Game.timerMinimum(bx0: Int, cx0: Int) {
    var bx = bx0; var cx = cx0
    var si = u(SLOTS_LAST)
    while (true) {
        val hi = u(PERIOD_HI + si); val lo = u(PERIOD_LO + si)
        if (cx > hi || (cx == hi && bx >= lo)) { bx = lo; cx = hi }
        si = (si - 2) and 0xffff
        if (si and 0x8000 != 0) break
    }
    setD(MIN_LO, bx); setD(MIN_HI, cx)
}

/**
 * 1a32:021b - removes the slot of handle [handle]; the last slot moves into its place. The original copies
 * the callback with the unscaled index and updates the handle table wrongly; kept as is (the game only ever
 * removes the last slot, where none of this happens).
 */
internal fun Game.timerRemove(handle: Int) {
    val di = handle
    var si = u(SLOTS_LAST)
    setD(SLOTS_LAST, (si - 2) and 0xffff)
    if (si != u(HANDLE_SLOT + di)) {
        val bx = u(HANDLE_SLOT + di)
        for (t in intArrayOf(PERIOD_LO, PERIOD_HI, COUNT_LO, COUNT_HI, CALLBACK, CALLBACK + 2, DUE)) setD(t + bx, u(t + si))
        val ax = u(SLOT_HANDLE + si)
        setD(SLOT_HANDLE + si, ax)
        si = ax
        setD(HANDLE_SLOT, si)
    }
    setD(HANDLE_SLOT + di, 0x8000)
    timerMinimum(0, 1)
}

/** 1a32:02c1 - new period cx:bx for the slot of handle [handle]. */
internal fun Game.timerSetPeriod(handle: Int, bx: Int, cx: Int) {
    val si = u(HANDLE_SLOT + handle)
    setD(PERIOD_LO + si, bx); setD(PERIOD_HI + si, cx)
    timerMinimum(bx, cx)
}

/**
 * 1a32:00d2 - the timer interrupt (INT 8 while the scheduler is installed): counts down all slots, calls the
 * due callbacks from the last slot down, then reprograms the PIT if the shortest period changed.
 */
fun Game.timerInterrupt() {
    val vector = far(mem.read16(0, 0x22), mem.read16(0, 0x20))
    if (vector != far(TIMER_SEG, 0x00d2)) return     // BIOS handler: nothing the port keeps
    val elapsed = (u(ELAPSED_HI).toLong() shl 16) or u(ELAPSED_LO).toLong()
    var si = u(SLOTS_LAST)
    while (true) {
        var count = ((u(COUNT_HI + si).toLong() shl 16) or u(COUNT_LO + si).toLong()) - elapsed
        count = count and 0xffffffffL
        if (count and 0x80000000L != 0L || count == 0L) {
            setD(DUE + si, 0x8000)
            count = (count + ((u(PERIOD_HI + si).toLong() shl 16) or u(PERIOD_LO + si).toLong())) and 0xffffffffL
        }
        setD(COUNT_LO + si, count.toInt() and 0xffff); setD(COUNT_HI + si, (count ushr 16).toInt())
        si = (si - 2) and 0xffff
        if (si and 0x8000 != 0) break
    }
    setD(ELAPSED_LO, u(MIN_LO)); setD(ELAPSED_HI, u(MIN_HI))
    si = u(SLOTS_LAST)
    while (true) {
        if (u(DUE + si) and 0x8000 != 0) {
            setD(DUE + si, 0)
            timerCallback(far(u(CALLBACK + 2 * si + 2), u(CALLBACK + 2 * si)))
        }
        si = (si - 2) and 0xffff
        if (si and 0x8000 != 0) break
    }
    if (u(PIT_DIVISOR) != u(MIN_LO)) host.timerDivisor(u(MIN_LO))
    setD(PIT_DIVISOR, u(MIN_LO))
}

private fun Game.timerCallback(address: Int) {
    when (address) {
        far(TIMER_SEG, 0x0099) -> {}                   // old INT 8 (BIOS clock): the host's business
        far(DRIVER_SEG, 0x1939) -> driverTick()
        far(DRIVER_SEG, 0x12e5) -> fadeTick()
        else -> error("timer callback %04x:%04x not ported".format(address ushr 16, address and 0xffff))
    }
}
