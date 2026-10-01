package ugh.desktop

import java.io.File
import java.security.MessageDigest
import java.util.Properties
import javax.swing.JFileChooser
import javax.swing.JOptionPane
import javax.swing.filechooser.FileNameExtensionFilter

/**
 * Finds the player's own copy of the original UGH.EXE (the port reads all game data from it and is not
 * distributed with it): the path given on the command line, the one remembered from last time, UGH.EXE next
 * to the application or in the working directory (also OLD\UGH.EXE), otherwise the player picks it once.
 * Only the version the port was made from is accepted.
 */
object GameFile {
    const val SHA256 = "ef93d2cd5eb558f6a7d0007e0109f2e9ee952dc125389d7a6256646087636d7c"

    private val settingsFile get() = File(saveDir, "ugh-port.properties")

    val saveDir = File(System.getenv("APPDATA") ?: System.getProperty("user.home"), "ugh-port")

    /** The original's bytes, or null when the player cancelled. */
    fun locate(argument: String?): ByteArray? {
        val candidates = buildList {
            if (argument != null) add(File(argument))
            remembered()?.let(::add)
            // jpackage tells the launcher's path; the original may sit next to UGH-port.exe
            System.getProperty("jpackage.app-path")?.let { add(File(File(it).absoluteFile.parentFile, "UGH.EXE")) }
            add(File("UGH.EXE")); add(File("OLD", "UGH.EXE"))
        }
        for (f in candidates) {
            val bytes = read(f) ?: continue
            if (sha256(bytes) == SHA256) { remember(f); return bytes }
            if (f.path == argument) wrongVersion(f)
        }
        while (true) {
            val chooser = JFileChooser().apply {
                dialogTitle = "Where is the original UGH.EXE?"
                fileFilter = FileNameExtensionFilter("UGH.EXE", "exe")
            }
            if (chooser.showOpenDialog(null) != JFileChooser.APPROVE_OPTION) return null
            val f = chooser.selectedFile
            val bytes = read(f) ?: continue
            if (sha256(bytes) == SHA256) { remember(f); return bytes }
            wrongVersion(f)
        }
    }

    private fun read(f: File) = if (f.isFile && f.length() < 4_000_000) f.readBytes() else null

    private fun wrongVersion(f: File) = JOptionPane.showMessageDialog(
        null,
        "${f.absolutePath} is not the version of UGH! this port was made from.\nExpected SHA-256: $SHA256",
        "UGH!", JOptionPane.ERROR_MESSAGE,
    )

    private fun remembered(): File? = try {
        Properties().apply { settingsFile.reader().use { load(it) } }.getProperty("exe")?.let(::File)
    } catch (_: Exception) { null }

    private fun remember(f: File) {
        try {
            saveDir.mkdirs()
            val p = Properties()
            if (settingsFile.isFile) settingsFile.reader().use { p.load(it) }
            p.setProperty("exe", f.absolutePath)
            settingsFile.writer().use { p.store(it, "UGH! port") }
        } catch (_: Exception) {
        }
    }

    private fun sha256(bytes: ByteArray) =
        MessageDigest.getInstance("SHA-256").digest(bytes).joinToString("") { "%02x".format(it) }
}
