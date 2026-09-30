package ugh.oracle

import ugh.core.hw.Memory
import ugh.core.hw.MzLoader

/**
 * "C:\" for the DOS file functions (UGH!.HI). Names are upper-case 8.3. With a [directory] the files are
 * loaded from it at start and written back whenever the program changes them (e.g. the high score table).
 */
class VirtualDisk(val files: MutableMap<String, ByteArray> = mutableMapOf(), val directory: java.io.File? = null) {
    init {
        directory?.listFiles()?.filter { it.isFile }?.forEach { files[it.name.uppercase()] = it.readBytes() }
    }

    fun changed(name: String) {
        val dir = directory ?: return
        dir.mkdirs()
        java.io.File(dir, name).writeBytes(files[name] ?: return)
    }
}

/**
 * A deterministic PC for running the original UGH.EXE: 1 MB RAM, VGA, 8259 PIC, 8253 PIT channel 0,
 * keyboard controller, and BIOS/DOS services implemented in Kotlin.
 *
 * Time is measured in emulated instructions ([ips] per second). Nothing depends on the host clock, so the
 * same program and the same input produce exactly the same run. There is no sound hardware: AdLib and
 * Sound Blaster probes read 0xFF, so the game runs with sound disabled.
 */
class Machine(exe: ByteArray, val disk: VirtualDisk = VirtualDisk(), val ips: Long = 20_000_000L) : Bus {
    /** Address space and VGA, shared type with the port (so state can be copied 1:1). */
    val memory = Memory()
    val mem = memory.ram
    val cpu = Cpu(this)
    val vga = memory.vga

    /** Emulated time in instructions; runs ahead of cpu.instructions when idle loops are skipped. */
    var time = 0L
        private set
    private var idle = 0L

    var exited = false
        private set
    var exitCode = -1
        private set
    val stdout = StringBuilder()

    // ------------------------------------------------------------ load

    val pspSeg = 0x0ff0
    val loadSeg = 0x1000

    init {
        val entry = MzLoader.load(exe, memory, loadSeg)

        // interrupt vectors point to host stubs F000:00nn (an IRET is placed there as well)
        for (n in 0 until 256) { poke16(n * 4, n); poke16(n * 4 + 2, 0xf000); mem[0xf0000 + n] = 0xcf.toByte() }

        // BIOS data area
        poke16(0x410, 0x0022)             // equipment: 80x25 colour
        poke16(0x413, 640)                // KB of conventional memory
        poke16(0x41a, 0x1e); poke16(0x41c, 0x1e)
        poke16(0x480, 0x1e); poke16(0x482, 0x3e)
        mem[0x449] = 3

        // environment block with the program path after the variables
        val envSeg = 0x0fe0
        val env = "COMSPEC=C:\\COMMAND.COM\u0000PATH=C:\\\u0000\u0000".toByteArray(Charsets.US_ASCII) +
            byteArrayOf(1, 0) + "C:\\UGH.EXE\u0000".toByteArray(Charsets.US_ASCII)
        System.arraycopy(env, 0, mem, envSeg * 16, env.size)

        // PSP
        val psp = pspSeg * 16
        mem[psp] = 0xcd.toByte(); mem[psp + 1] = 0x20
        poke16(psp + 2, 0xa000)
        poke16(psp + 0x2c, envSeg)
        mem[psp + 0x80] = 0; mem[psp + 0x81] = 0x0d

        cpu.cs = entry.cs; cpu.ip = entry.ip
        cpu.ss = entry.ss; cpu.sp = entry.sp
        cpu.ds = pspSeg; cpu.es = pspSeg
        cpu.flags = 0x0202
    }

    private fun poke16(a: Int, v: Int) { mem[a] = v.toByte(); mem[a + 1] = (v shr 8).toByte() }
    fun peek8(a: Int) = mem[a].toInt() and 0xff
    fun peek16(a: Int) = peek8(a) or (peek8(a + 1) shl 8)
    fun read8(seg: Int, off: Int) = read8(((seg shl 4) + off) and 0xfffff)
    fun read16(seg: Int, off: Int) = read8(seg, off) or (read8(seg, off + 1) shl 8)
    fun write8(seg: Int, off: Int, v: Int) = write8(((seg shl 4) + off) and 0xfffff, v)
    fun write16(seg: Int, off: Int, v: Int) { write8(seg, off, v and 0xff); write8(seg, off + 1, (v shr 8) and 0xff) }

    // ------------------------------------------------------------ bus

    override fun read8(addr: Int): Int = memory.read8(addr)

    override fun write8(addr: Int, v: Int) = memory.write8(addr, v)

    /** CS:IP of instructions that only poll the VGA status; reading there may skip idle time. */
    val pollSites = HashSet<Int>()

    override fun in8(port: Int): Int = when (port) {
        0x20 -> pic.isr
        0x21 -> pic.imr
        0x40 -> 0
        0x60 -> { kbd.outputFull = false; kbd.latch }
        0x61 -> 0
        0x64 -> if (kbd.outputFull) 0x1d else 0x1c
        0x3da -> { vga.resetFlipFlop(); retraceStatus() }
        in 0x3c0..0x3cf, 0x3d5 -> vga.input(port)
        else -> 0xff // joystick 0x201, AdLib 0x388, Sound Blaster: not present
    }

    override fun out8(port: Int, v: Int) {
        when (port) {
            0x20 -> pic.eoi(v)
            0x21 -> pic.imr = v
            0x43 -> pit.control(v)
            0x40 -> {
                pit.data(v, time)
                if (pit.pendingReset >= 0) { pit.nextIrq = time + pit.period(ips); pit.pendingReset = -1; nextEvent = 0 }
            }
            in 0x3c0..0x3cf, 0x3d4, 0x3d5 -> vga.out(port, v)
        }
    }

    override fun halt() { skipTo(nextEventTime()) }

    // ------------------------------------------------------------ timing

    /** VGA 320x200 @ 70.086 Hz: 800 dots x 449 lines at 25.175 MHz, vertical retrace = 2 lines. */
    fun frameStart(n: Long) = n * ips * 359_200L / 25_175_000L
    private val retraceLength = ips * 1_600L / 25_175_000L

    /** Number of vertical retraces started so far. */
    var vgaFrame = 0L
        private set

    private fun inRetrace() = time - frameStart(vgaFrame) < retraceLength && vgaFrame > 0

    private fun retraceStatus(): Int {
        var r = inRetrace()
        val site = (cpu.lastInstructionCs shl 4) + cpu.lastInstructionIp
        if (site in pollSites) {
            // the program spins on this read: jump straight to the next change
            if (!r) { skipTo(minOf(frameStart(vgaFrame + 1), nextEventTime())); r = inRetrace() }
            else { skipTo(minOf(frameStart(vgaFrame) + retraceLength, nextEventTime())); r = inRetrace() }
        }
        return (if (r) 0x08 else 0) or (if (r || time and 1L == 0L) 0x01 else 0)
    }

    private fun skipTo(t: Long) { if (t > time) { idle += t - time; time = t; processEvents(); nextEvent = 0 } }

    private fun nextEventTime(): Long {
        var t = frameStart(vgaFrame + 1)
        if (pit.nextIrq in (time + 1) until t) t = pit.nextIrq
        if (kbd.queue.isNotEmpty() && kbd.nextTime > time && kbd.nextTime < t) t = kbd.nextTime
        return maxOf(t, time + 1)
    }

    /** Listeners called at every vertical retrace start with the new [vgaFrame]. */
    val frameListeners = mutableListOf<(Long) -> Unit>()

    private fun processEvents() {
        while (time >= frameStart(vgaFrame + 1)) {
            vgaFrame++
            for (l in frameListeners) l(vgaFrame)
        }
        while (time >= pit.nextIrq) { pic.request(0); pit.nextIrq += pit.period(ips) }
        if (kbd.queue.isNotEmpty() && time >= kbd.nextTime && !kbd.outputFull && pic.isr and 0x02 == 0) {
            kbd.latch = kbd.queue.removeFirst()
            kbd.outputFull = true
            kbd.nextTime = time + ips / 1000
            pic.request(1)
        }
    }

    // ------------------------------------------------------------ run

    private val breakpoints = HashMap<Int, MutableList<() -> Unit>>()
    private val bpFlags = BooleanArray(0x100000)

    /** Calls [action] every time execution reaches seg:off (before the instruction runs). */
    fun onExecute(seg: Int, off: Int, action: () -> Unit) {
        val a = ((seg shl 4) + off) and 0xfffff
        breakpoints.getOrPut(a) { mutableListOf() } += action
        bpFlags[a] = true
    }

    var stopRequested = false

    /** While set, hardware interrupts stay pending (used to check a routine in isolation). */
    var irqHold = false

    /** Runs until the program exits, [stop] is requested, or [maxInstructions] have executed. */
    fun run(maxInstructions: Long = Long.MAX_VALUE) {
        stopRequested = false
        val limit = if (maxInstructions == Long.MAX_VALUE) Long.MAX_VALUE else cpu.instructions + maxInstructions
        while (!exited && !stopRequested && cpu.instructions < limit) {
            if (cpu.iff && !cpu.irqInhibit && !irqHold) {
                val irq = pic.next()
                if (irq >= 0) { pic.acknowledge(irq); cpu.interrupt(8 + irq) }
            }
            val a = ((cpu.cs shl 4) + cpu.ip) and 0xfffff
            if (bpFlags[a]) {
                breakpoints[a]?.forEach { it() }
                if (stopRequested || exited) break
            }
            stepOnce()
        }
    }

    private fun stepOnce() {
        try {
            cpu.step()
        } catch (e: CpuException) {
            throw CpuException("${e.message} (time $time, frame $vgaFrame)")
        }
        time = cpu.instructions + idle
        if (time >= nextEvent) { processEvents(); nextEvent = nextEventTime() }
    }

    /**
     * Services all deliverable hardware interrupts right now, running each handler to its IRET (breakpoints
     * are not triggered inside). Used at frame boundaries of the differential tests, so that interrupts
     * held back during a checked frame take effect before the next frame starts.
     */
    fun serviceInterruptsNow() {
        while (cpu.iff && !irqHold) {
            val irq = pic.next()
            if (irq < 0) return
            pic.acknowledge(irq)
            val retCs = cpu.cs; val retIp = cpu.ip; val retSp = cpu.sp
            cpu.interrupt(8 + irq)
            var guard = 0L
            while (!(cpu.cs == retCs && cpu.ip == retIp && cpu.sp == retSp)) {
                if (cpu.iff && !cpu.irqInhibit) {
                    val nested = pic.next()
                    if (nested >= 0) { pic.acknowledge(nested); cpu.interrupt(8 + nested) }
                }
                stepOnce()
                if (++guard > 50_000_000L) throw CpuException("interrupt handler %d did not return".format(irq))
            }
        }
    }

    /** Cached time of the next device event; 0 forces a recalculation. */
    private var nextEvent = 0L

    fun stop() { stopRequested = true }

    // ------------------------------------------------------------ devices

    class Pic {
        var imr = 0xb8 // as left by the BIOS: timer, keyboard, cascade, floppy enabled
        var irr = 0
        var isr = 0
        fun request(n: Int) { irr = irr or (1 shl n) }
        fun next(): Int {
            val pending = irr and imr.inv()
            if (pending == 0) return -1
            for (i in 0..7) {
                if (isr and (1 shl i) != 0) return -1
                if (pending and (1 shl i) != 0) return i
            }
            return -1
        }
        fun acknowledge(n: Int) { irr = irr and (1 shl n).inv(); isr = isr or (1 shl n) }
        fun eoi(v: Int) {
            if (v == 0x20) { for (i in 0..7) if (isr and (1 shl i) != 0) { isr = isr and (1 shl i).inv(); return } }
            else if (v and 0xf8 == 0x60) isr = isr and (1 shl (v and 7)).inv()
        }
    }

    class Pit {
        var divisor = 0x10000
        var nextIrq = 0L
        private var lowByte = -1
        private var accessMode = 3
        fun period(ips: Long) = divisor.toLong() * ips / 1_193_182L
        fun control(v: Int) { if (v shr 6 == 0) { accessMode = (v shr 4) and 3; lowByte = -1 } }
        fun data(v: Int, now: Long) {
            when (accessMode) {
                1 -> set(v, now)
                2 -> set(v shl 8, now)
                else -> if (lowByte < 0) lowByte = v else { set(lowByte or (v shl 8), now); lowByte = -1 }
            }
        }
        private fun set(d: Int, now: Long) { divisor = if (d == 0) 0x10000 else d; pendingReset = now }
        var pendingReset = -1L
    }

    class Keyboard {
        val queue = ArrayDeque<Int>()
        var latch = 0
        var outputFull = false
        var nextTime = 0L
    }

    val pic = Pic()
    val pit = Pit()
    val kbd = Keyboard()

    init {
        pit.nextIrq = pit.period(ips)
    }

    /** Queues raw scancodes (make and break codes, 0xE0 prefixes included). */
    fun scancodes(vararg codes: Int) {
        for (c in codes) kbd.queue.addLast(c and 0xff)
        if (kbd.nextTime < time) kbd.nextTime = time
        nextEvent = 0
    }

    // ------------------------------------------------------------ host interrupts

    private val stackedFlags get() = ((cpu.ss shl 4) + cpu.sp + 4) and 0xfffff
    private fun setStackedFlag(mask: Int, on: Boolean) {
        val a = stackedFlags
        val f = peek16(a)
        poke16(a, if (on) f or mask else f and mask.inv())
    }
    private fun setCarry(on: Boolean) = setStackedFlag(0x0001, on)
    private fun setZero(on: Boolean) = setStackedFlag(0x0040, on)

    override fun hostInterrupt(n: Int): Boolean {
        when (n) {
            0x08 -> biosTimer()
            0x09 -> biosKeyboard()
            0x10 -> biosVideo()
            0x16 -> return biosKeys()
            0x1a -> { cpu.cx = peek16(0x46e); cpu.dx = peek16(0x46c); cpu.ax = (cpu.ax and 0xff00) or peek8(0x470); mem[0x470] = 0 }
            0x21 -> dos()
            0x00 -> throw CpuException("divide error at ${"%04x:%04x".format(peek16(stackedFlags - 2), peek16(stackedFlags - 4))}")
            else -> {} // unused vectors: plain IRET
        }
        return true
    }

    private fun biosTimer() {
        var t = peek16(0x46c).toLong() or (peek16(0x46e).toLong() shl 16)
        t++
        if (t >= 0x1800b0) { t = 0; mem[0x470] = 1 }
        poke16(0x46c, (t and 0xffff).toInt()); poke16(0x46e, (t shr 16).toInt())
        pic.eoi(0x20)
    }

    private val asciiOf = mapOf(
        0x01 to 0x1b, 0x0e to 0x08, 0x1c to 0x0d, 0x39 to 0x20,
        0x02 to '1'.code, 0x03 to '2'.code, 0x04 to '3'.code, 0x05 to '4'.code, 0x06 to '5'.code,
        0x07 to '6'.code, 0x08 to '7'.code, 0x09 to '8'.code, 0x0a to '9'.code, 0x0b to '0'.code,
    ) + "QWERTYUIOP".mapIndexed { i, c -> 0x10 + i to c.lowercaseChar().code } +
        "ASDFGHJKL".mapIndexed { i, c -> 0x1e + i to c.lowercaseChar().code } +
        "ZXCVBNM".mapIndexed { i, c -> 0x2c + i to c.lowercaseChar().code }

    private var e0Prefix = false

    private fun biosKeyboard() {
        val sc = kbd.latch
        kbd.outputFull = false
        if (sc == 0xe0) { e0Prefix = true; pic.eoi(0x20); return }
        val ext = e0Prefix
        e0Prefix = false
        if (sc and 0x80 == 0) {
            val word = (sc shl 8) or (if (ext) 0 else asciiOf[sc] ?: 0)
            val tail = peek16(0x41c)
            val next = if (tail + 2 >= 0x3e) 0x1e else tail + 2
            if (next != peek16(0x41a)) { poke16(0x400 + tail, word); poke16(0x41c, next) }
        }
        pic.eoi(0x20)
    }

    private fun biosKeys(): Boolean {
        val head = peek16(0x41a)
        val empty = head == peek16(0x41c)
        when (cpu.ax shr 8) {
            0x00, 0x10 -> {
                if (empty) { skipTo(nextEventTime()); return false }
                cpu.ax = peek16(0x400 + head)
                poke16(0x41a, if (head + 2 >= 0x3e) 0x1e else head + 2)
            }
            0x01, 0x11 -> { setZero(empty); if (!empty) cpu.ax = peek16(0x400 + head) }
            0x02 -> cpu.ax = cpu.ax and 0xff00
        }
        return true
    }

    private fun biosVideo() {
        when (cpu.ax shr 8) {
            0x00 -> { vga.setMode(cpu.ax and 0x7f); mem[0x449] = (cpu.ax and 0x7f).toByte() }
            0x0e -> stdout.append((cpu.ax and 0xff).toChar())
            0x0f -> { val m = vga.mode; cpu.ax = ((if (m == 0x13) 40 else 80) shl 8) or m; cpu.bx = cpu.bx and 0xff }
        }
    }

    // ------------------------------------------------------------ DOS

    private val memBlocks = mutableListOf(pspSeg to (0xa000 - pspSeg)) // segment -> paragraphs (no MCB chain)
    private class Handle(val name: String, var data: ByteArray, var pos: Int, val write: Boolean)
    private val handles = HashMap<Int, Handle>()

    private fun asciiz(seg: Int, off: Int): String {
        val sb = StringBuilder()
        var o = off
        while (true) { val c = read8(seg, o++); if (c == 0) break; sb.append(c.toChar()) }
        return sb.toString()
    }

    private fun fileName(path: String) = path.substringAfterLast('\\').substringAfterLast(':').uppercase()

    private fun ok() = setCarry(false)
    private fun fail(code: Int) { cpu.ax = code; setCarry(true) }

    /** When set, every DOS call is reported here (for debugging). */
    var dosTrace: ((String) -> Unit)? = null

    private fun dos() {
        val ah = cpu.ax shr 8
        val al = cpu.ax and 0xff
        dosTrace?.invoke("INT21 AX=%04x BX=%04x CX=%04x DX=%04x DS=%04x ES=%04x from %04x:%04x".format(
            cpu.ax, cpu.bx, cpu.cx, cpu.dx, cpu.ds, cpu.es, peek16(stackedFlags - 2), peek16(stackedFlags - 4)))
        when (ah) {
            0x02 -> stdout.append((cpu.dx and 0xff).toChar())
            0x09 -> { var o = cpu.dx; while (true) { val c = read8(cpu.ds, o++); if (c == '$'.code) break; stdout.append(c.toChar()) } }
            0x19 -> cpu.ax = (cpu.ax and 0xff00) or 2
            0x1a -> {}
            0x25 -> { poke16(al * 4, cpu.dx); poke16(al * 4 + 2, cpu.ds) }
            0x2a -> { cpu.cx = 1994; cpu.dx = 0x0101; cpu.ax = (cpu.ax and 0xff00) or 6 }
            0x2c -> { cpu.cx = 0; cpu.dx = 0 }
            0x30 -> { cpu.ax = 0x0005; cpu.bx = 0; cpu.cx = 0 }
            0x33 -> cpu.dx = cpu.dx and 0xff00
            0x35 -> { cpu.bx = peek16(al * 4); cpu.es = peek16(al * 4 + 2) }
            0x3c, 0x3d -> {
                val name = fileName(asciiz(cpu.ds, cpu.dx))
                val existing = disk.files[name]
                if (ah == 0x3d && existing == null) { fail(2); return }
                val h = (5..40).first { it !in handles }
                handles[h] = Handle(name, if (ah == 0x3c) ByteArray(0) else existing!!.copyOf(), 0, ah == 0x3c || al and 3 != 0)
                if (ah == 0x3c) disk.files[name] = ByteArray(0)
                cpu.ax = h; ok()
            }
            0x3e -> { val h = handles.remove(cpu.bx); if (h == null && cpu.bx > 4) fail(6) else ok() }
            0x3f -> {
                val h = handles[cpu.bx] ?: kotlin.run { if (cpu.bx == 0) { cpu.ax = 0; ok() } else fail(6); return }
                val n = minOf(cpu.cx, h.data.size - h.pos).coerceAtLeast(0)
                for (i in 0 until n) write8(cpu.ds, cpu.dx + i, h.data[h.pos + i].toInt())
                h.pos += n; cpu.ax = n; ok()
            }
            0x40 -> {
                val bytes = ByteArray(cpu.cx) { read8(cpu.ds, cpu.dx + it).toByte() }
                if (cpu.bx in 1..2) { stdout.append(String(bytes, Charsets.ISO_8859_1)); cpu.ax = cpu.cx; ok(); return }
                val h = handles[cpu.bx] ?: kotlin.run { fail(6); return }
                val end = h.pos + bytes.size
                if (end > h.data.size) h.data = h.data.copyOf(end)
                System.arraycopy(bytes, 0, h.data, h.pos, bytes.size)
                h.pos = end
                disk.files[h.name] = h.data.copyOf()
                disk.changed(h.name)
                cpu.ax = bytes.size; ok()
            }
            0x42 -> {
                val h = handles[cpu.bx] ?: kotlin.run { fail(6); return }
                val off = (cpu.cx shl 16) or cpu.dx
                h.pos = when (al) { 0 -> off; 1 -> h.pos + off; else -> h.data.size + off }
                cpu.dx = h.pos shr 16; cpu.ax = h.pos and 0xffff; ok()
            }
            0x44 -> when (al) {
                0x00 -> { cpu.dx = if (cpu.bx <= 4) 0x80d3 else 0x0002; ok() }
                else -> fail(1)
            }
            0x47 -> { write8(cpu.ds, cpu.si, 0); ok() }
            0x48 -> {
                val need = cpu.bx
                val end = memBlocks.maxOf { it.first + it.second }
                if (end + need <= 0xa000) { memBlocks += end to need; cpu.ax = end; ok() }
                else { cpu.bx = 0xa000 - end; fail(8) }
            }
            0x49 -> { memBlocks.removeIf { it.first == cpu.es && it.first != pspSeg }; ok() }
            0x4a -> {
                val i = memBlocks.indexOfFirst { it.first == cpu.es }
                if (i < 0) { fail(9); return }
                val limit = memBlocks.filter { it.first > cpu.es }.minOfOrNull { it.first } ?: 0xa000
                if (cpu.es + cpu.bx <= limit) { memBlocks[i] = cpu.es to cpu.bx; ok() }
                else { cpu.bx = limit - cpu.es; fail(8) }
            }
            0x4c -> { exited = true; exitCode = al; stopRequested = true }
            0x58 -> ok()
            0x67 -> ok()
            else -> throw CpuException("unimplemented DOS function %02x (AX=%04x) at %04x:%04x".format(ah, cpu.ax, peek16(stackedFlags - 2), peek16(stackedFlags - 4)))
        }
    }
}
