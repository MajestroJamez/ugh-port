package ugh.oracle

import org.junit.jupiter.api.Disabled
import org.junit.jupiter.api.Test
import java.io.File

/** Diagnostic aid: prints DOS calls and the last instructions before the program stops. */
@Disabled("diagnostic aid, run manually")
class DebugTraceTest {
    private val exe = File(System.getProperty("ugh.exe")).readBytes()

    @Test
    fun `trace startup`() {
        val ugh = OriginalUgh(exe)
        val m = ugh.machine
        m.dosTrace = { println(it) }
        val ring = ArrayDeque<String>()
        var n = 0
        while (!m.exited && n < 3_000_000) {
            ring.addLast("%04x:%04x AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x DS=%04x ES=%04x SS=%04x SP=%04x".format(
                m.cpu.cs, m.cpu.ip, m.cpu.ax, m.cpu.bx, m.cpu.cx, m.cpu.dx, m.cpu.si, m.cpu.di, m.cpu.ds, m.cpu.es, m.cpu.ss, m.cpu.sp))
            if (ring.size > 40) ring.removeFirst()
            m.run(1)
            n++
        }
        println("exited=${m.exited} code=${m.exitCode} after $n steps, game frames ${ugh.gameFrame}")
        ring.forEach { println(it) }
    }
}
