package ugh.desktop

import ugh.oracle.Machine
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
import java.util.concurrent.ConcurrentLinkedQueue
import javax.swing.JFrame
import javax.swing.JOptionPane
import javax.swing.JPanel
import javax.swing.SwingUtilities
import kotlin.system.exitProcess

/**
 * Desktop window for UGH!. For now it runs the ORIGINAL program in the built-in emulator (no sound);
 * the ported core will replace it once complete.
 *
 * Usage: desktop [path\to\UGH.EXE]   (default OLD\UGH.EXE; the high score table is kept in %APPDATA%\ugh-port)
 */
fun main(args: Array<String>) {
    val exeFile = File(args.firstOrNull() ?: "OLD/UGH.EXE")
    if (!exeFile.isFile) {
        JOptionPane.showMessageDialog(null, "UGH.EXE not found: ${exeFile.absolutePath}", "UGH!", JOptionPane.ERROR_MESSAGE)
        exitProcess(1)
    }
    val saveDir = File(System.getenv("APPDATA") ?: System.getProperty("user.home"), "ugh-port")
    val ugh = OriginalUgh(exeFile.readBytes(), VirtualDisk(directory = saveDir))
    SwingUtilities.invokeLater { Window(ugh).start() }
}

private class Window(private val ugh: OriginalUgh) {
    private val machine: Machine = ugh.machine
    private val keys = ConcurrentLinkedQueue<Int>()
    private val held = HashSet<Int>()
    private val image = BufferedImage(320, 200, BufferedImage.TYPE_INT_RGB)
    private val rgb = IntArray(320 * 200)
    @Volatile private var running = true

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

    fun start() {
        val frame = JFrame("UGH! (original running in the built-in emulator)")
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
        Thread(::loop, "ugh-emulator").apply { isDaemon = true }.start()
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

    private fun loop() {
        val period = 1_000_000_000.0 / 70.086
        var next = System.nanoTime().toDouble()
        var target = machine.vgaFrame + 1
        machine.frameListeners += { if (it >= target) machine.stop() }
        while (running && !machine.exited) {
            while (true) { val k = keys.poll() ?: break; machine.scancodes(k) }
            target = machine.vgaFrame + 1
            machine.run(50_000_000L)
            present()
            next += period
            val sleep = (next - System.nanoTime()) / 1_000_000.0
            if (sleep > 0) Thread.sleep(sleep.toLong(), ((sleep % 1) * 1_000_000).toInt())
            else if (sleep < -200) next = System.nanoTime().toDouble()   // fell behind: resynchronise
        }
        exitProcess(0)
    }

    private fun present() {
        val px = machine.vga.renderIndexed()
        val dac = machine.vga.dac
        for (i in px.indices) {
            val c = (px[i].toInt() and 0xff) * 3
            fun ch(v: Int) = (v shl 2) or (v shr 4)
            rgb[i] = (ch(dac[c]) shl 16) or (ch(dac[c + 1]) shl 8) or ch(dac[c + 2])
        }
        synchronized(image) { image.setRGB(0, 0, 320, 200, rgb, 0, 320) }
        panel.repaint()
    }
}
