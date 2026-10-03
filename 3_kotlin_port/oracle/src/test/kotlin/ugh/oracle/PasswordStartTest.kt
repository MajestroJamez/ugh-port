package ugh.oracle

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ugh.extractor.Exe
import ugh.extractor.Levels
import java.io.File

class PasswordStartTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val out = File(System.getProperty("ugh.out"), "password").apply { mkdirs() }

    @Test
    fun `a level password from the EXE starts that level`() {
        val levels = Levels.extract(Exe(exe))
        val ugh = OriginalUgh(exe)
        val level = 4
        val ok = ugh.startGame(password = levels.passwordsOnePlayer[level], log = ::println)
        val (img, pal) = ugh.screenshot()
        img.writePng(File(out, "after_start.png"), pal)
        assertTrue(ok, "level play not reached")
        assertEquals(level, ugh.dgroup16(0x261c))
    }
}
