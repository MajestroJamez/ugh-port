package ugh.desktop

import ugh.core.audio.SoundTimeline
import ugh.core.game.Game
import ugh.core.game.Host
import ugh.core.game.StopGame
import ugh.core.game.keyEvent
import ugh.core.game.runProgram
import ugh.core.game.timerInterrupt
import ugh.core.hw.Memory
import ugh.core.hw.MzLoader
import ugh.core.hw.Vga
import ugh.oracle.OriginalUgh
import ugh.oracle.VirtualDisk
import java.awt.Color
import java.awt.Dimension
import java.awt.Graphics
import java.awt.Graphics2D
import java.awt.RenderingHints
import java.awt.event.KeyAdapter
import java.awt.event.KeyEvent
import java.awt.image.BufferedImage
import java.io.File
import java.time.LocalTime
import java.util.concurrent.ConcurrentLinkedQueue
import javax.swing.JFrame
import javax.swing.JOptionPane
import javax.swing.JPanel
import javax.swing.SwingUtilities
import kotlin.system.exitProcess

/**
 * Desktop window for UGH!: runs the port (game code, graphics, AdLib sound through the OPL2 synthesizer).
 * With --original it runs the original program in the built-in emulator instead (no sound), for comparison.
 *
 * Usage: desktop [--original] [path\to\UGH.EXE]   (default OLD\UGH.EXE; the high score table is kept in
 * %APPDATA%\ugh-port, the same file for both)
 */
fun main(args: Array<String>) {
    val original = "--original" in args
    val exeFile = File(args.firstOrNull { !it.startsWith("--") } ?: "OLD/UGH.EXE")
    if (!exeFile.isFile) {
        JOptionPane.showMessageDialog(null, "UGH.EXE not found: ${exeFile.absolutePath}", "UGH!", JOptionPane.ERROR_MESSAGE)
        exitProcess(1)
    }
    val saveDir = File(System.getenv("APPDATA") ?: System.getProperty("user.home"), "ugh-port")
    val exe = exeFile.readBytes()
    SwingUtilities.invokeLater {
        if (original) {
            val ugh = OriginalUgh(exe, VirtualDisk(directory = saveDir))
            val window = Window("UGH! (original running in the built-in emulator)")
            window.open { OriginalRunner(ugh, window).run() }
        } else {
            val window = Window("UGH!")
            window.open { PortRunner(exe, saveDir, window).run() }
        }
    }
}

/** The window: a 4:3 picture of the 320x200 screen and the keyboard as PC scancodes. */
private class Window(private val title: String) {
    val keys = ConcurrentLinkedQueue<Int>()
    private val held = HashSet<Int>()
    private val image = BufferedImage(320, 200, BufferedImage.TYPE_INT_RGB)
    private val rgb = IntArray(320 * 200)

    private val panel = object : JPanel() {
        override fun paintComponent(g: Graphics) {
            super.paintComponent(g)
            val g2 = g as Graphics2D
            g2.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR)
            // 320x200 was shown on a 4:3 screen: keep that aspect ratio, letterbox the rest
            val w = width; val h = height
            val scale = minOf(w / 320.0, h / 240.0)
            val dw = (320 * scale).toInt(); val dh = (240 * scale).toInt()
            synchronized(image) { g2.drawImage(image, (w - dw) / 2, (h - dh) / 2, dw, dh, null) }
        }
    }

    fun open(game: () -> Unit) {
        val frame = JFrame(title)
        panel.background = Color.BLACK
        panel.preferredSize = Dimension(960, 720)
        panel.isFocusable = true
        panel.addKeyListener(object : KeyAdapter() {
            override fun keyPressed(e: KeyEvent) = key(e, true)
            override fun keyReleased(e: KeyEvent) = key(e, false)
        })
        frame.contentPane.add(panel)
        frame.defaultCloseOperation = JFrame.EXIT_ON_CLOSE
        frame.pack()
        frame.setLocationRelativeTo(null)
        frame.isVisible = true
        panel.requestFocusInWindow()
        Thread({ game(); exitProcess(0) }, "ugh-game").apply { isDaemon = true }.start()
    }

    private fun key(e: KeyEvent, down: Boolean) {
        val code = Keys.of(e) ?: return
        val id = code.make or (if (code.extended) 0x100 else 0)
        if (down && !held.add(id)) return          // no typematic repeat
        if (!down) held.remove(id)
        if (code.extended) keys.add(0xe0)
        keys.add(if (down) code.make else code.make or 0x80)
        e.consume()
    }

    fun present(vga: Vga) {
        val px = vga.renderIndexed()
        val dac = vga.dac
        for (i in px.indices) {
            val c = (px[i].toInt() and 0xff) * 3
            fun ch(v: Int) = (v shl 2) or (v shr 4)
            rgb[i] = (ch(dac[c]) shl 16) or (ch(dac[c + 1]) shl 8) or ch(dac[c + 2])
        }
        synchronized(image) { image.setRGB(0, 0, 320, 200, rgb, 0, 320) }
        panel.repaint()
    }
}

/** Keeps a loop at the VGA frame rate (70.086 Hz). */
private class FrameClock {
    private val period = 1_000_000_000.0 / SoundTimeline.FRAME_HZ
    private var next = System.nanoTime().toDouble()

    fun waitFrame() {
        next += period
        val sleep = (next - System.nanoTime()) / 1_000_000.0
        if (sleep > 0) Thread.sleep(sleep.toLong(), ((sleep % 1) * 1_000_000).toInt())
        else if (sleep < -200) next = System.nanoTime().toDouble()   // fell behind: resynchronise
    }

    /** Time passed outside the frame loop (the game's busy wait for the sound): start counting anew. */
    fun resync() { next = System.nanoTime().toDouble() }
}

/** The port: its frames are the original's retrace waits, its sound runs on the PIT time line. */
private class PortRunner(exe: ByteArray, private val saveDir: File, private val window: Window) : Host {
    private val mem = Memory().also { MzLoader.load(exe, it) }
    private val game = Game(mem, this)
    private val audio = AudioOut()
    private val timeline = SoundTimeline(audio::sample)
    private val clock = FrameClock()
    private var waited = false

    fun run() {
        try {
            game.runProgram()
        } catch (_: StopGame) {
        } finally {
            audio.close()
        }
    }

    override fun frame() {
        if (waited) { clock.resync(); waited = false }
        timeline.frame { game.timerInterrupt() }
        audio.endFrame()
        window.present(mem.vga)
        clock.waitFrame()
        while (true) game.keyEvent(window.keys.poll() ?: break)
    }

    override fun readFile(name: String): ByteArray? = File(saveDir, name).takeIf { it.isFile }?.readBytes()

    override fun writeFile(name: String, data: ByteArray) {
        saveDir.mkdirs()
        File(saveDir, name).writeBytes(data)
    }

    override fun clockSeconds() = LocalTime.now().second

    override fun adlibPresent() = true

    override fun adlib(reg: Int, value: Int) = timeline.write(reg, value)

    override fun timerDivisor(divisor: Int) { timeline.divisor = divisor }

    override fun timerWait() {
        timeline.untilNextTick()
        audio.flushBlocking()
        waited = true
    }
}

/** The original program in the emulator, one VGA frame per window frame. */
private class OriginalRunner(ugh: OriginalUgh, private val window: Window) {
    private val machine = ugh.machine

    fun run() {
        val clock = FrameClock()
        var target = machine.vgaFrame + 1
        machine.frameListeners += { if (it >= target) machine.stop() }
        while (!machine.exited) {
            while (true) { val k = window.keys.poll() ?: break; machine.scancodes(k) }
            target = machine.vgaFrame + 1
            machine.run(50_000_000L)
            window.present(machine.vga)
            clock.waitFrame()
        }
    }
}
