package ugh.verify

import org.junit.jupiter.api.Disabled
import org.junit.jupiter.api.Test
import ugh.oracle.OriginalUgh
import java.io.File

/** Diagnostic: screenshots of a cheat pilot run, to see where the game stops. */
@Disabled("diagnostic aid, run manually")
class CheatPilotScreensTest {
    @Test
    fun screens() {
        val exe = File(System.getProperty("ugh.exe")).readBytes()
        val out = File(System.getProperty("ugh.out"), "cheat").apply { mkdirs() }
        val ugh = OriginalUgh(exe)
        check(ugh.startGame())
        val pilot = CheatPilot(ugh, 1).also { it.attach() }
        for (i in 1..24) {
            ugh.runGameFrames(250)
            val (img, pal) = ugh.screenshot()
            img.writePng(File(out, "f%05d.png".format(ugh.gameFrame)), pal)
            println("frame ${ugh.gameFrame}: fade ${ugh.dgroupS16(0x27a8)} step ${ugh.dgroupS16(0x27aa)} level ${ugh.dgroup16(0x261c)} ${pilot.actions}")
        }
    }
}
