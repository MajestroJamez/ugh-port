package ugh.desktop

import java.awt.event.KeyEvent

/** Maps Java key events to PC/XT (set 1) scancodes; extended keys carry the 0xE0 prefix. */
object Keys {
    data class Code(val make: Int, val extended: Boolean = false)

    fun of(e: KeyEvent): Code? {
        val right = e.keyLocation == KeyEvent.KEY_LOCATION_RIGHT
        val numpad = e.keyLocation == KeyEvent.KEY_LOCATION_NUMPAD
        return when (e.keyCode) {
            KeyEvent.VK_UP -> if (numpad) Code(0x48) else Code(0x48, true)
            KeyEvent.VK_DOWN -> if (numpad) Code(0x50) else Code(0x50, true)
            KeyEvent.VK_LEFT -> if (numpad) Code(0x4b) else Code(0x4b, true)
            KeyEvent.VK_RIGHT -> if (numpad) Code(0x4d) else Code(0x4d, true)
            KeyEvent.VK_KP_UP, KeyEvent.VK_NUMPAD8 -> Code(0x48)
            KeyEvent.VK_KP_DOWN, KeyEvent.VK_NUMPAD2 -> Code(0x50)
            KeyEvent.VK_KP_LEFT, KeyEvent.VK_NUMPAD4 -> Code(0x4b)
            KeyEvent.VK_KP_RIGHT, KeyEvent.VK_NUMPAD6 -> Code(0x4d)
            KeyEvent.VK_NUMPAD0 -> Code(0x52)
            KeyEvent.VK_INSERT -> Code(0x52, true)
            KeyEvent.VK_DELETE -> Code(0x53, true)
            KeyEvent.VK_HOME -> Code(0x47, true)
            KeyEvent.VK_END -> Code(0x4f, true)
            KeyEvent.VK_PAGE_UP -> Code(0x49, true)
            KeyEvent.VK_PAGE_DOWN -> Code(0x51, true)
            KeyEvent.VK_CONTROL -> Code(0x1d, right)
            KeyEvent.VK_ENTER -> Code(0x1c, numpad)
            KeyEvent.VK_SHIFT -> Code(if (right) 0x36 else 0x2a)
            KeyEvent.VK_ALT -> Code(0x38, right)
            KeyEvent.VK_ESCAPE -> Code(0x01)
            KeyEvent.VK_BACK_SPACE -> Code(0x0e)
            KeyEvent.VK_TAB -> Code(0x0f)
            KeyEvent.VK_SPACE -> Code(0x39)
            in KeyEvent.VK_F1..KeyEvent.VK_F10 -> Code(0x3b + (e.keyCode - KeyEvent.VK_F1))
            KeyEvent.VK_F11 -> Code(0x57)
            KeyEvent.VK_F12 -> Code(0x58)
            KeyEvent.VK_MINUS -> Code(0x0c)
            KeyEvent.VK_EQUALS -> Code(0x0d)
            KeyEvent.VK_PERIOD -> Code(0x34)
            KeyEvent.VK_COMMA -> Code(0x33)
            KeyEvent.VK_1 -> Code(0x02); KeyEvent.VK_2 -> Code(0x03); KeyEvent.VK_3 -> Code(0x04)
            KeyEvent.VK_4 -> Code(0x05); KeyEvent.VK_5 -> Code(0x06); KeyEvent.VK_6 -> Code(0x07)
            KeyEvent.VK_7 -> Code(0x08); KeyEvent.VK_8 -> Code(0x09); KeyEvent.VK_9 -> Code(0x0a)
            KeyEvent.VK_0 -> Code(0x0b)
            in KeyEvent.VK_A..KeyEvent.VK_Z -> Code(LETTERS[e.keyCode - KeyEvent.VK_A])
            else -> null
        }
    }

    /** Scancodes of A..Z. */
    private val LETTERS = intArrayOf(
        0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
        0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c,
    )
}
