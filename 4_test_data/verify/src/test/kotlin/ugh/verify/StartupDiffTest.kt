package ugh.verify

import org.junit.jupiter.api.Assumptions.assumeTrue
import org.junit.jupiter.api.Test
import ugh.core.hw.Memory
import ugh.core.hw.MzLoader
import ugh.oracle.OriginalUgh
import java.io.File

/** Exploration: what the C runtime start-up changes in memory before main (113b:000d). */
class StartupDiffTest {
    @Test
    fun `memory changed by the C runtime start-up`() {
        assumeTrue(System.getProperty("ugh.explore") != null, "exploration only")
        val exe = File(System.getProperty("ugh.exe")).readBytes()
        val ugh = OriginalUgh(exe)
        val dos = ArrayList<String>()
        ugh.machine.dosTrace = { dos += it }
        check(ugh.runUntil(0x113b, 0x000d, 100))
        val fresh = Memory(); MzLoader.load(exe, fresh)
        val a = ugh.machine.memory.ram; val b = fresh.ram
        var i = 0
        val c = ugh.machine.cpu
        println("registers at main: ss:sp %04x:%04x ds %04x es %04x".format(c.ss, c.sp, c.ds, c.es))
        while (i < 0xa0000) {
            if (a[i] != b[i]) {
                var j = i
                while (j < 0xa0000 && (a[j] != b[j] || (j + 1 < 0xa0000 && a[j + 1] != b[j + 1]) || (j + 2 < 0xa0000 && a[j + 2] != b[j + 2]))) j++
                val bytes = (i until minOf(j, i + 24)).joinToString(" ") { "%02x".format(a[it]) }
                println("%05x..%05x (%d): %s".format(i, j - 1, j - i, bytes))
                i = j
            } else i++
        }
        dos.forEach(::println)
    }
}
