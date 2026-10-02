package ugh.extractor

import ugh.core.data.Ice
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.io.TempDir
import java.io.File

class ExtractorTest {

    private val exe = Exe(File(System.getProperty("ugh.exe")).readBytes())

    @Test
    fun `exe is the known version`() {
        assertEquals(KNOWN_EXE_SHA256, sha256(exe.bytes))
    }

    /** Golden hashes produced by the independent JS port (re/tools/unice.js); the pictures were checked visually. */
    @Test
    fun `ice depacker matches reference output`() {
        val golden = mapOf(
            0x2a38 to "0ec89a86aad97e0385775624ea4e9b0146e63b9ea66c8d8a57cbc496c448ebfe",
            0x3bf6 to "ffad6e3a4e1ef7e3afdc9ffdb1171ea16b2533c4692d48d99a9b6de0ea76ff18",
            0x3d9a to "aa4e6a85ebdac3bfe52cc46c4b4f7df7d68dcb4905905681f69a2765c93b324b",
            0x3f9e to "a4fb59c14c668389e3d6d7574edfa9e91e00cda30e9c5e3c7b5cd8c513c098c8",
            0x3fb7 to "78740c230a998d6fd4945990e51655aee5124918542150ddb275823bd2fa10f4",
        )
        for ((seg, hash) in golden) assertEquals(hash, sha256(Ice.unpack(exe.from(seg))), "segment ${seg.hex()}")
    }

    @Test
    fun `level tables`() {
        val set = Levels.extract(exe)
        assertEquals(69, set.onePlayer.size)
        assertEquals(81, set.team.size)
        assertEquals(81, set.levels.size)
        // game texts are checked by hash only, so that no original content lives in this repository
        assertEquals("7c604e76712f3253143b5e3beb505182cc12d806a10d7fd39e7c8e3100e0fba5", sha256(set.passwordsOnePlayer.joinToString("\n")))
        assertEquals("5901c2759e44f2b68b06502556a3773982c79a29e0867b018aae552b98786c08", sha256(set.passwordsTeam.joinToString("\n")))
        assertEquals("1151fdd52e1ee3f14c962ba4323bc433a62d4f8cabe8d6d486897c9291436467", sha256(set.levels.joinToString("\n") { it.caption }))
        val first = set.levels.first { it.record == set.onePlayer.first() }
        assertEquals(3, first.pads.size)
        assertEquals(listOf(RouteStop(0, 100), RouteStop(2, 300), RouteStop(1, 500), RouteStop(0, 200)), first.passengers[0].route)
        assertEquals(0x1400, first.start1X)
    }

    @Test
    fun `sprite table`() {
        val table = Sprites.table(exe)
        assertEquals(664, table.size)
        assertEquals(SpriteEntry(0, 0, 16, 12, 0x2e1f), table[0])
        assertEquals(SpriteEntry(1, 0xc0, 16, 12, 0x2e1f), table[1])
    }

    @Test
    fun `full extraction writes all assets`(@TempDir dir: File) {
        val m = extractAll(exe, dir)
        assertEquals(664, m.sprites)
        assertTrue(File(dir, "levels.json").length() > 0)
        assertTrue(File(dir, "pictures/title_ugh.png").exists())
        assertEquals(Layout.DGROUP_INIT_SIZE.toLong(), File(dir, "dgroup.bin").length())
    }

    /** The collision masks themselves are checked against the background page of the port in :verify. */
    @Test
    fun `simulation data reads back`(@TempDir dir: File) {
        val blocks = Sim.blocks(exe)
        val file = File(dir, "ugh-sim.bin")
        Sim.write(blocks, file)
        val read = Sim.read(file)
        assertEquals(blocks.map { Triple(it.name, it.seg, it.off) }, read.map { Triple(it.name, it.seg, it.off) })
        assertTrue(blocks.zip(read).all { (a, b) -> a.data.contentEquals(b.data) })
        assertEquals(listOf("DGROUP", "MAPS", "SPRITES"), read.take(3).map { it.name })
        assertEquals(100 * 320, read[1].data.size)
        val masks = read.filter { it.name == "MASK" }
        assertEquals(Levels.extract(exe).levels.map { it.record }, masks.map { it.off })
        assertTrue(masks.all { it.data.size == Sim.MASK_ROW_BYTES * Sim.MASK_HEIGHT })
    }
}
