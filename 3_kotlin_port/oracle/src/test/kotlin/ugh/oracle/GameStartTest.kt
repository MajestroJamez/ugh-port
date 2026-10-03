package ugh.oracle

import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import java.io.File

class GameStartTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val out = File(System.getProperty("ugh.out"), "start").apply { mkdirs() }

    private fun OriginalUgh.shot(name: String) {
        val (img, pal) = screenshot()
        img.writePng(File(out, "$name.png"), pal)
    }

    @Test
    fun `power-on to level 1 play`() {
        val ugh = OriginalUgh(exe)
        assertTrue(ugh.startGame(log = ::println), "level play not reached")
        println("level play reached at game frame ${ugh.gameFrame}, level index ${ugh.dgroup16(0x261c)}")
        for (i in 1..6) {
            ugh.runGameFrames(60)
            ugh.shot("play_%02d".format(i))
        }
    }
}
