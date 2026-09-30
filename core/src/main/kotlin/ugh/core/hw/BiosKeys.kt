package ugh.core.hw

/**
 * Scancode to BIOS keyboard word (scancode << 8 | ASCII) as the BIOS INT 9 handler of the emulated PC and the
 * port's BIOS keyboard mode produce it: letters lower case (no shift handling), digits, Esc, Backspace, Enter,
 * Space; extended (0xE0) and other keys give ASCII 0. Break codes produce nothing.
 */
object BiosKeys {
    private val ascii: Map<Int, Int> = mapOf(
        0x01 to 0x1b, 0x0e to 0x08, 0x1c to 0x0d, 0x39 to 0x20,
        0x02 to '1'.code, 0x03 to '2'.code, 0x04 to '3'.code, 0x05 to '4'.code, 0x06 to '5'.code,
        0x07 to '6'.code, 0x08 to '7'.code, 0x09 to '8'.code, 0x0a to '9'.code, 0x0b to '0'.code,
    ) + "QWERTYUIOP".mapIndexed { i, c -> 0x10 + i to c.lowercaseChar().code } +
        "ASDFGHJKL".mapIndexed { i, c -> 0x1e + i to c.lowercaseChar().code } +
        "ZXCVBNM".mapIndexed { i, c -> 0x2c + i to c.lowercaseChar().code }

    /** Converts a make code; [extended] = preceded by 0xE0. */
    fun word(scancode: Int, extended: Boolean): Int = (scancode shl 8) or (if (extended) 0 else ascii[scancode] ?: 0)
}
