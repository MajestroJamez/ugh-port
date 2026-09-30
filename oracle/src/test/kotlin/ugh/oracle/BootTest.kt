package ugh.oracle

import org.junit.jupiter.api.Assertions.assertFalse
import org.junit.jupiter.api.Test
import java.io.File

class BootTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()
    private val out = File(System.getProperty("ugh.out"), "boot").apply { mkdirs() }

    @Test
    fun `original boots through the intro`() {
        val ugh = OriginalUgh(exe)
        val t0 = System.nanoTime()
        for (i in 1..12) {
            ugh.runGameFrames(100)
            val (img, pal) = ugh.screenshot()
            img.writePng(File(out, "frame%05d.png".format(ugh.gameFrame)), pal)
        }
        val ms = (System.nanoTime() - t0) / 1_000_000
        println("game frames ${ugh.gameFrame}, vga frames ${ugh.machine.vgaFrame}, instructions ${ugh.machine.cpu.instructions}, ${ms} ms")
        println("stdout: ${ugh.machine.stdout}")
        assertFalse(ugh.machine.exited, "program exited with ${ugh.machine.exitCode}: ${ugh.machine.stdout}")
    }
}
