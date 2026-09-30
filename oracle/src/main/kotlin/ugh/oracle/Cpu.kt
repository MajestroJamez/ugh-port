package ugh.oracle

/** What the CPU needs from the machine around it. */
interface Bus {
    fun read8(addr: Int): Int
    fun write8(addr: Int, v: Int)
    fun in8(port: Int): Int
    fun out8(port: Int, v: Int)
    fun in16(port: Int): Int = in8(port) or (in8(port + 1) shl 8)
    fun out16(port: Int, v: Int) { out8(port, v and 0xff); out8(port + 1, v shr 8) }

    /**
     * Called when execution reaches F000:00nn, the stub of a host-implemented interrupt nn.
     * Returns true when done (the CPU then IRETs), false to retry later (e.g. INT 16h waiting for a key).
     */
    fun hostInterrupt(n: Int): Boolean

    /** Called on HLT; the machine should advance time to the next interrupt. */
    fun halt()
}

class CpuException(msg: String) : RuntimeException(msg)

/**
 * 80286 real-mode interpreter (80186 instruction set, 286 FLAGS behaviour - UGH.EXE refuses to run on
 * an 8086/186), sufficient for UGH.EXE (Borland C++ runtime + hand written TASM code).
 * Undefined flags follow what DOSBox does where it matters (AF after logic ops = 0).
 */
class Cpu(private val bus: Bus) {
    // general registers, always kept in 0..0xffff
    var ax = 0; var cx = 0; var dx = 0; var bx = 0
    var sp = 0; var bp = 0; var si = 0; var di = 0
    var es = 0; var cs = 0; var ss = 0; var ds = 0
    var ip = 0

    var cf = false; var pf = false; var af = false; var zf = false
    var sf = false; var tf = false; var iff = false; var df = false; var of = false

    /** Executed instructions (a REP string instruction counts once per iteration). */
    var instructions = 0L

    /** Linear address of the instruction being executed, for diagnostics. */
    var lastInstructionCs = 0
    var lastInstructionIp = 0

    /** Interrupts are inhibited for one instruction after MOV SS / POP SS / STI. */
    var irqInhibit = false

    private var segOverride = -1
    private var opcodeIp = 0

    // ---------------------------------------------------------------- flags

    var flags: Int
        get() = 0x0002 or (if (cf) 0x0001 else 0) or (if (pf) 0x0004 else 0) or (if (af) 0x0010 else 0) or
            (if (zf) 0x0040 else 0) or (if (sf) 0x0080 else 0) or (if (tf) 0x0100 else 0) or
            (if (iff) 0x0200 else 0) or (if (df) 0x0400 else 0) or (if (of) 0x0800 else 0) // 286: bits 12-15 are 0 in real mode
        set(v) {
            cf = v and 0x0001 != 0; pf = v and 0x0004 != 0; af = v and 0x0010 != 0; zf = v and 0x0040 != 0
            sf = v and 0x0080 != 0; tf = v and 0x0100 != 0; iff = v and 0x0200 != 0; df = v and 0x0400 != 0
            of = v and 0x0800 != 0
        }

    // ---------------------------------------------------------------- registers

    fun reg16(i: Int): Int = when (i) {
        0 -> ax; 1 -> cx; 2 -> dx; 3 -> bx; 4 -> sp; 5 -> bp; 6 -> si; else -> di
    }

    fun setReg16(i: Int, v: Int) {
        val x = v and 0xffff
        when (i) { 0 -> ax = x; 1 -> cx = x; 2 -> dx = x; 3 -> bx = x; 4 -> sp = x; 5 -> bp = x; 6 -> si = x; else -> di = x }
    }

    fun reg8(i: Int): Int = when (i) {
        0 -> ax and 0xff; 1 -> cx and 0xff; 2 -> dx and 0xff; 3 -> bx and 0xff
        4 -> ax shr 8; 5 -> cx shr 8; 6 -> dx shr 8; else -> bx shr 8
    }

    fun setReg8(i: Int, v: Int) {
        val x = v and 0xff
        when (i) {
            0 -> ax = (ax and 0xff00) or x; 1 -> cx = (cx and 0xff00) or x
            2 -> dx = (dx and 0xff00) or x; 3 -> bx = (bx and 0xff00) or x
            4 -> ax = (ax and 0xff) or (x shl 8); 5 -> cx = (cx and 0xff) or (x shl 8)
            6 -> dx = (dx and 0xff) or (x shl 8); else -> bx = (bx and 0xff) or (x shl 8)
        }
    }

    fun sreg(i: Int): Int = when (i and 3) { 0 -> es; 1 -> cs; 2 -> ss; else -> ds }

    private fun setSreg(i: Int, v: Int) {
        when (i and 3) { 0 -> es = v; 1 -> cs = v; 2 -> { ss = v; irqInhibit = true }; else -> ds = v }
    }

    // ---------------------------------------------------------------- memory

    fun lin(seg: Int, off: Int) = ((seg shl 4) + (off and 0xffff)) and 0xfffff

    fun rd8(seg: Int, off: Int) = bus.read8(lin(seg, off))
    fun rd16(seg: Int, off: Int) = bus.read8(lin(seg, off)) or (bus.read8(lin(seg, off + 1)) shl 8)
    fun wr8(seg: Int, off: Int, v: Int) = bus.write8(lin(seg, off), v and 0xff)
    fun wr16(seg: Int, off: Int, v: Int) {
        bus.write8(lin(seg, off), v and 0xff)
        bus.write8(lin(seg, off + 1), (v shr 8) and 0xff)
    }

    private fun fetch8(): Int { val v = rd8(cs, ip); ip = (ip + 1) and 0xffff; return v }
    private fun fetch16(): Int { val v = rd16(cs, ip); ip = (ip + 2) and 0xffff; return v }
    private fun fetchS8(): Int = fetch8().toByte().toInt()

    fun push(v: Int) { sp = (sp - 2) and 0xffff; wr16(ss, sp, v) }
    fun pop(): Int { val v = rd16(ss, sp); sp = (sp + 2) and 0xffff; return v }

    // ---------------------------------------------------------------- ModR/M

    private var mMod = 0
    private var mReg = 0
    private var mRm = 0
    private var eaSeg = 0
    private var eaOff = 0

    private fun modrm() {
        val b = fetch8()
        mMod = b shr 6; mReg = (b shr 3) and 7; mRm = b and 7
        if (mMod == 3) return
        var defSeg = ds
        var off = when (mRm) {
            0 -> bx + si
            1 -> bx + di
            2 -> { defSeg = ss; bp + si }
            3 -> { defSeg = ss; bp + di }
            4 -> si
            5 -> di
            6 -> if (mMod == 0) fetch16() else { defSeg = ss; bp }
            else -> bx
        }
        when (mMod) {
            1 -> off += fetchS8()
            2 -> off += fetch16()
        }
        eaOff = off and 0xffff
        eaSeg = if (segOverride >= 0) sreg(segOverride) else defSeg
    }

    private fun rm8(): Int = if (mMod == 3) reg8(mRm) else rd8(eaSeg, eaOff)
    private fun rm16(): Int = if (mMod == 3) reg16(mRm) else rd16(eaSeg, eaOff)
    private fun setRm8(v: Int) { if (mMod == 3) setReg8(mRm, v) else wr8(eaSeg, eaOff, v) }
    private fun setRm16(v: Int) { if (mMod == 3) setReg16(mRm, v) else wr16(eaSeg, eaOff, v) }

    private fun defaultSeg() = if (segOverride >= 0) sreg(segOverride) else ds

    // ---------------------------------------------------------------- ALU

    private fun parity(v: Int) = Integer.bitCount(v and 0xff) and 1 == 0

    private fun szp8(r: Int) { zf = r and 0xff == 0; sf = r and 0x80 != 0; pf = parity(r) }
    private fun szp16(r: Int) { zf = r and 0xffff == 0; sf = r and 0x8000 != 0; pf = parity(r) }

    private fun add8(a: Int, b: Int, c: Int): Int {
        val r = a + b + c
        cf = r > 0xff; af = (a xor b xor r) and 0x10 != 0; of = (a xor r) and (b xor r) and 0x80 != 0
        szp8(r); return r and 0xff
    }

    private fun add16(a: Int, b: Int, c: Int): Int {
        val r = a + b + c
        cf = r > 0xffff; af = (a xor b xor r) and 0x10 != 0; of = (a xor r) and (b xor r) and 0x8000 != 0
        szp16(r); return r and 0xffff
    }

    private fun sub8(a: Int, b: Int, c: Int): Int {
        val r = a - b - c
        cf = r < 0; af = (a xor b xor r) and 0x10 != 0; of = (a xor b) and (a xor r) and 0x80 != 0
        szp8(r); return r and 0xff
    }

    private fun sub16(a: Int, b: Int, c: Int): Int {
        val r = a - b - c
        cf = r < 0; af = (a xor b xor r) and 0x10 != 0; of = (a xor b) and (a xor r) and 0x8000 != 0
        szp16(r); return r and 0xffff
    }

    private fun logic8(r: Int): Int { cf = false; of = false; af = false; szp8(r); return r and 0xff }
    private fun logic16(r: Int): Int { cf = false; of = false; af = false; szp16(r); return r and 0xffff }

    /** op: 0 ADD 1 OR 2 ADC 3 SBB 4 AND 5 SUB 6 XOR 7 CMP; returns result (CMP returns a unchanged). */
    private fun alu8(op: Int, a: Int, b: Int): Int = when (op) {
        0 -> add8(a, b, 0)
        1 -> logic8(a or b)
        2 -> add8(a, b, if (cf) 1 else 0)
        3 -> sub8(a, b, if (cf) 1 else 0)
        4 -> logic8(a and b)
        5 -> sub8(a, b, 0)
        6 -> logic8(a xor b)
        else -> { sub8(a, b, 0); a }
    }

    private fun alu16(op: Int, a: Int, b: Int): Int = when (op) {
        0 -> add16(a, b, 0)
        1 -> logic16(a or b)
        2 -> add16(a, b, if (cf) 1 else 0)
        3 -> sub16(a, b, if (cf) 1 else 0)
        4 -> logic16(a and b)
        5 -> sub16(a, b, 0)
        6 -> logic16(a xor b)
        else -> { sub16(a, b, 0); a }
    }

    private fun inc8(a: Int): Int { val c = cf; val r = add8(a, 1, 0); cf = c; return r }
    private fun dec8(a: Int): Int { val c = cf; val r = sub8(a, 1, 0); cf = c; return r }
    private fun inc16(a: Int): Int { val c = cf; val r = add16(a, 1, 0); cf = c; return r }
    private fun dec16(a: Int): Int { val c = cf; val r = sub16(a, 1, 0); cf = c; return r }

    /** Shift/rotate group (C0/C1/D0-D3). op: 0 ROL 1 ROR 2 RCL 3 RCR 4 SHL 5 SHR 6 SAL(=SHL) 7 SAR. */
    private fun shift(op: Int, v: Int, countIn: Int, bits: Int): Int {
        val count = countIn and 0x1f
        if (count == 0) return v
        val mask = if (bits == 8) 0xff else 0xffff
        val msb = if (bits == 8) 0x80 else 0x8000
        var r = v
        when (op) {
            0 -> { repeat(count) { val b = r and msb != 0; r = ((r shl 1) or (if (b) 1 else 0)) and mask; cf = b }; of = (r and msb != 0) != cf }
            1 -> { repeat(count) { val b = r and 1 != 0; r = (r shr 1) or (if (b) msb else 0); cf = b }; of = (r and msb != 0) != (r and (msb shr 1) != 0) }
            2 -> { repeat(count) { val b = r and msb != 0; r = ((r shl 1) or (if (cf) 1 else 0)) and mask; cf = b }; of = (r and msb != 0) != cf }
            3 -> { repeat(count) { val b = r and 1 != 0; r = (r shr 1) or (if (cf) msb else 0); cf = b }; of = (r and msb != 0) != (r and (msb shr 1) != 0) }
            4, 6 -> {
                cf = if (count <= bits) (v shl (count - 1)) and msb != 0 else false
                r = if (count < 32) (v shl count) and mask else 0
                of = (r and msb != 0) != cf
                if (bits == 8) szp8(r) else szp16(r); af = false
            }
            5 -> {
                cf = if (count <= bits) (v shr (count - 1)) and 1 != 0 else false
                r = if (count < 32) v ushr count else 0
                of = v and msb != 0
                if (bits == 8) szp8(r) else szp16(r); af = false
            }
            else -> {
                val sv = if (bits == 8) v.toByte().toInt() else v.toShort().toInt()
                val c = minOf(count, bits)
                cf = (sv shr (c - 1)) and 1 != 0
                r = (sv shr c) and mask
                of = false
                if (bits == 8) szp8(r) else szp16(r); af = false
            }
        }
        return r and mask
    }

    // ---------------------------------------------------------------- interrupts

    /** Software or hardware interrupt entry through the IVT. */
    fun interrupt(n: Int) {
        push(flags)
        iff = false; tf = false
        push(cs); push(ip)
        ip = rd16(0, n * 4)
        cs = rd16(0, n * 4 + 2)
    }

    fun iret() {
        ip = pop(); cs = pop(); flags = pop()
    }

    // ---------------------------------------------------------------- string ops

    private fun strDelta(size: Int) = if (df) -size else size

    private fun stringOp(op: Int, rep: Int) {
        // rep: 0 none, 0xf3 REP/REPE, 0xf2 REPNE
        val size = if (op and 1 == 0) 1 else 2
        val d = strDelta(size)
        val src = defaultSeg()
        fun once() {
            when (op) {
                0xa4, 0xa5 -> { // MOVS
                    if (size == 1) wr8(es, di, rd8(src, si)) else wr16(es, di, rd16(src, si))
                    si = (si + d) and 0xffff; di = (di + d) and 0xffff
                }
                0xa6, 0xa7 -> { // CMPS
                    if (size == 1) sub8(rd8(src, si), rd8(es, di), 0) else sub16(rd16(src, si), rd16(es, di), 0)
                    si = (si + d) and 0xffff; di = (di + d) and 0xffff
                }
                0xaa, 0xab -> { // STOS
                    if (size == 1) wr8(es, di, ax and 0xff) else wr16(es, di, ax)
                    di = (di + d) and 0xffff
                }
                0xac, 0xad -> { // LODS
                    if (size == 1) setReg8(0, rd8(src, si)) else ax = rd16(src, si)
                    si = (si + d) and 0xffff
                }
                0xae, 0xaf -> { // SCAS
                    if (size == 1) sub8(ax and 0xff, rd8(es, di), 0) else sub16(ax, rd16(es, di), 0)
                    di = (di + d) and 0xffff
                }
                0x6c, 0x6d -> { // INS
                    if (size == 1) wr8(es, di, bus.in8(dx)) else wr16(es, di, bus.in16(dx))
                    di = (di + d) and 0xffff
                }
                0x6e, 0x6f -> { // OUTS
                    if (size == 1) bus.out8(dx, rd8(src, si)) else bus.out16(dx, rd16(src, si))
                    si = (si + d) and 0xffff
                }
            }
        }
        if (rep == 0) { once(); return }
        val compares = op in 0xa6..0xa7 || op in 0xae..0xaf
        while (cx != 0) {
            once()
            cx = (cx - 1) and 0xffff
            instructions++
            if (compares && ((rep == 0xf3 && !zf) || (rep == 0xf2 && zf))) break
        }
    }

    // ---------------------------------------------------------------- execution

    private fun jcc(cond: Boolean) { val d = fetchS8(); if (cond) ip = (ip + d) and 0xffff }

    private fun cond(c: Int): Boolean = when (c) {
        0 -> of; 1 -> !of; 2 -> cf; 3 -> !cf; 4 -> zf; 5 -> !zf; 6 -> cf || zf; 7 -> !cf && !zf
        8 -> sf; 9 -> !sf; 10 -> pf; 11 -> !pf; 12 -> sf != of; 13 -> sf == of
        14 -> zf || sf != of; else -> !zf && sf == of
    }

    private fun unsupported(op: Int): Nothing =
        throw CpuException("unsupported opcode %02x at %04x:%04x".format(op, cs, opcodeIp))

    /** Executes one instruction (including its prefixes). */
    fun step() {
        if (cs == 0xf000 && ip < 0x100) {
            // host-implemented interrupt stub: the handler works on the stacked frame, then IRET
            lastInstructionCs = cs; lastInstructionIp = ip
            val done = bus.hostInterrupt(ip)
            if (done && cs == 0xf000 && ip < 0x100) iret()
            instructions++
            return
        }
        irqInhibit = false
        segOverride = -1
        var rep = 0
        lastInstructionCs = cs
        lastInstructionIp = ip
        opcodeIp = ip
        var op: Int
        while (true) {
            op = fetch8()
            when (op) {
                0x26 -> segOverride = 0
                0x2e -> segOverride = 1
                0x36 -> segOverride = 2
                0x3e -> segOverride = 3
                0xf0 -> {}
                0xf2, 0xf3 -> rep = op
                else -> break
            }
        }
        instructions++
        when (op) {
            in 0x00..0x3f -> if (op and 7 < 6) {
                val alu = op shr 3
                when (op and 7) {
                    0 -> { modrm(); val r = alu8(alu, rm8(), reg8(mReg)); if (alu != 7) setRm8(r) }
                    1 -> { modrm(); val r = alu16(alu, rm16(), reg16(mReg)); if (alu != 7) setRm16(r) }
                    2 -> { modrm(); val r = alu8(alu, reg8(mReg), rm8()); if (alu != 7) setReg8(mReg, r) }
                    3 -> { modrm(); val r = alu16(alu, reg16(mReg), rm16()); if (alu != 7) setReg16(mReg, r) }
                    4 -> { val r = alu8(alu, ax and 0xff, fetch8()); if (alu != 7) setReg8(0, r) }
                    5 -> { val r = alu16(alu, ax, fetch16()); if (alu != 7) ax = r }
                }
            } else when (op) {
                0x06 -> push(es); 0x07 -> es = pop()
                0x0e -> push(cs); 0x16 -> push(ss); 0x17 -> { ss = pop(); irqInhibit = true }
                0x1e -> push(ds); 0x1f -> ds = pop()
                else -> unsupported(op) // DAA/DAS/AAA/AAS are not used by UGH.EXE
            }
            in 0x40..0x47 -> setReg16(op - 0x40, inc16(reg16(op - 0x40)))
            in 0x48..0x4f -> setReg16(op - 0x48, dec16(reg16(op - 0x48)))
            in 0x50..0x57 -> push(reg16(op - 0x50))
            in 0x58..0x5f -> setReg16(op - 0x58, pop())
            0x60 -> { val t = sp; push(ax); push(cx); push(dx); push(bx); push(t); push(bp); push(si); push(di) }
            0x61 -> { di = pop(); si = pop(); bp = pop(); pop(); bx = pop(); dx = pop(); cx = pop(); ax = pop() }
            0x68 -> push(fetch16())
            0x69, 0x6b -> {
                modrm()
                val a = rm16().toShort().toInt()
                val b = if (op == 0x69) fetch16().toShort().toInt() else fetchS8()
                val r = a * b
                setReg16(mReg, r)
                cf = r != r.toShort().toInt(); of = cf
            }
            0x6a -> push(fetchS8() and 0xffff)
            in 0x6c..0x6f -> stringOp(op, rep)
            in 0x70..0x7f -> jcc(cond(op - 0x70))
            0x80, 0x82 -> { modrm(); val r = alu8(mReg, rm8(), fetch8()); if (mReg != 7) setRm8(r) }
            0x81 -> { modrm(); val r = alu16(mReg, rm16(), fetch16()); if (mReg != 7) setRm16(r) }
            0x83 -> { modrm(); val r = alu16(mReg, rm16(), fetchS8() and 0xffff); if (mReg != 7) setRm16(r) }
            0x84 -> { modrm(); logic8(rm8() and reg8(mReg)) }
            0x85 -> { modrm(); logic16(rm16() and reg16(mReg)) }
            0x86 -> { modrm(); val t = rm8(); setRm8(reg8(mReg)); setReg8(mReg, t) }
            0x87 -> { modrm(); val t = rm16(); setRm16(reg16(mReg)); setReg16(mReg, t) }
            0x88 -> { modrm(); setRm8(reg8(mReg)) }
            0x89 -> { modrm(); setRm16(reg16(mReg)) }
            0x8a -> { modrm(); setReg8(mReg, rm8()) }
            0x8b -> { modrm(); setReg16(mReg, rm16()) }
            0x8c -> { modrm(); setRm16(sreg(mReg)) }
            0x8d -> { modrm(); setReg16(mReg, eaOff) }
            0x8e -> { modrm(); setSreg(mReg, rm16()) }
            0x8f -> { modrm(); val v = pop(); setRm16(v) }
            0x90 -> {}
            in 0x91..0x97 -> { val t = ax; ax = reg16(op - 0x90); setReg16(op - 0x90, t) }
            0x98 -> ax = (ax and 0xff).toByte().toInt() and 0xffff
            0x99 -> dx = if (ax and 0x8000 != 0) 0xffff else 0
            0x9a -> { val o = fetch16(); val s = fetch16(); push(cs); push(ip); cs = s; ip = o }
            0x9b -> {}
            0x9c -> push(flags)
            0x9d -> flags = pop()
            0x9e -> { val f = ax shr 8; flags = (flags and 0xff00) or (f and 0xd5) }
            0x9f -> setReg8(4, flags and 0xff)
            0xa0 -> setReg8(0, rd8(defaultSeg(), fetch16()))
            0xa1 -> ax = rd16(defaultSeg(), fetch16())
            0xa2 -> wr8(defaultSeg(), fetch16(), ax and 0xff)
            0xa3 -> wr16(defaultSeg(), fetch16(), ax)
            in 0xa4..0xa7, in 0xaa..0xaf -> stringOp(op, rep)
            0xa8 -> logic8((ax and 0xff) and fetch8())
            0xa9 -> logic16(ax and fetch16())
            in 0xb0..0xb7 -> setReg8(op - 0xb0, fetch8())
            in 0xb8..0xbf -> setReg16(op - 0xb8, fetch16())
            0xc0 -> { modrm(); val n = fetch8(); setRm8(shift(mReg, rm8(), n, 8)) }
            0xc1 -> { modrm(); val n = fetch8(); setRm16(shift(mReg, rm16(), n, 16)) }
            0xc2 -> { val n = fetch16(); ip = pop(); sp = (sp + n) and 0xffff }
            0xc3 -> ip = pop()
            0xc4 -> { modrm(); setReg16(mReg, rd16(eaSeg, eaOff)); es = rd16(eaSeg, eaOff + 2) }
            0xc5 -> { modrm(); setReg16(mReg, rd16(eaSeg, eaOff)); ds = rd16(eaSeg, eaOff + 2) }
            0xc6 -> { modrm(); setRm8(fetch8()) }
            0xc7 -> { modrm(); setRm16(fetch16()) }
            0xc8 -> {
                val size = fetch16(); val level = fetch8() and 0x1f
                push(bp)
                val frame = sp
                if (level > 0) {
                    for (i in 1 until level) { bp = (bp - 2) and 0xffff; push(rd16(ss, bp)) }
                    push(frame)
                }
                bp = frame
                sp = (sp - size) and 0xffff
            }
            0xc9 -> { sp = bp; bp = pop() }
            0xca -> { val n = fetch16(); ip = pop(); cs = pop(); sp = (sp + n) and 0xffff }
            0xcb -> { ip = pop(); cs = pop() }
            0xcc -> interrupt(3)
            0xcd -> interrupt(fetch8())
            0xce -> if (of) interrupt(4)
            0xcf -> iret()
            0xd0 -> { modrm(); setRm8(shift(mReg, rm8(), 1, 8)) }
            0xd1 -> { modrm(); setRm16(shift(mReg, rm16(), 1, 16)) }
            0xd2 -> { modrm(); setRm8(shift(mReg, rm8(), cx and 0xff, 8)) }
            0xd3 -> { modrm(); setRm16(shift(mReg, rm16(), cx and 0xff, 16)) }
            0xd4 -> { val b = fetch8(); if (b == 0) { divideError(); return }; val al = ax and 0xff; ax = ((al / b) shl 8) or (al % b); szp8(ax) }
            0xd5 -> { val b = fetch8(); val r = ((ax and 0xff) + (ax shr 8) * b) and 0xff; ax = r; szp8(r) }
            0xd7 -> setReg8(0, rd8(defaultSeg(), bx + (ax and 0xff)))
            0xe0 -> { val d = fetchS8(); cx = (cx - 1) and 0xffff; if (cx != 0 && !zf) ip = (ip + d) and 0xffff }
            0xe1 -> { val d = fetchS8(); cx = (cx - 1) and 0xffff; if (cx != 0 && zf) ip = (ip + d) and 0xffff }
            0xe2 -> { val d = fetchS8(); cx = (cx - 1) and 0xffff; if (cx != 0) ip = (ip + d) and 0xffff }
            0xe3 -> { val d = fetchS8(); if (cx == 0) ip = (ip + d) and 0xffff }
            0xe4 -> setReg8(0, bus.in8(fetch8()))
            0xe5 -> ax = bus.in16(fetch8())
            0xe6 -> bus.out8(fetch8(), ax and 0xff)
            0xe7 -> bus.out16(fetch8(), ax)
            0xe8 -> { val d = fetch16(); push(ip); ip = (ip + d) and 0xffff }
            0xe9 -> { val d = fetch16(); ip = (ip + d) and 0xffff }
            0xea -> { val o = fetch16(); val s = fetch16(); cs = s; ip = o }
            0xeb -> { val d = fetchS8(); ip = (ip + d) and 0xffff }
            0xec -> setReg8(0, bus.in8(dx))
            0xed -> ax = bus.in16(dx)
            0xee -> bus.out8(dx, ax and 0xff)
            0xef -> bus.out16(dx, ax)
            0xf4 -> bus.halt()
            0xf5 -> cf = !cf
            0xf6 -> group3(8)
            0xf7 -> group3(16)
            0xf8 -> cf = false; 0xf9 -> cf = true
            0xfa -> iff = false; 0xfb -> { iff = true; irqInhibit = true }
            0xfc -> df = false; 0xfd -> df = true
            0xfe -> {
                modrm()
                when (mReg) { 0 -> setRm8(inc8(rm8())); 1 -> setRm8(dec8(rm8())); else -> unsupported(op) }
            }
            0xff -> {
                modrm()
                when (mReg) {
                    0 -> setRm16(inc16(rm16()))
                    1 -> setRm16(dec16(rm16()))
                    2 -> { val t = rm16(); push(ip); ip = t }
                    3 -> { val o = rd16(eaSeg, eaOff); val s = rd16(eaSeg, eaOff + 2); push(cs); push(ip); cs = s; ip = o }
                    4 -> ip = rm16()
                    5 -> { ip = rd16(eaSeg, eaOff); cs = rd16(eaSeg, eaOff + 2) }
                    6 -> push(rm16())
                    else -> unsupported(op)
                }
            }
            else -> unsupported(op)
        }
    }

    private fun divideError() {
        ip = opcodeIp // 186+: the return address points at the faulting instruction
        interrupt(0)
    }

    private fun group3(bits: Int) {
        modrm()
        if (bits == 8) {
            val v = rm8()
            when (mReg) {
                0, 1 -> logic8(v and fetch8())
                2 -> setRm8(v.inv())
                3 -> { val r = sub8(0, v, 0); setRm8(r); cf = v != 0 }
                4 -> { val r = (ax and 0xff) * v; ax = r and 0xffff; cf = r and 0xff00 != 0; of = cf; szp8(r) }
                5 -> { val r = (ax and 0xff).toByte().toInt() * v.toByte().toInt(); ax = r and 0xffff; cf = r != r.toByte().toInt(); of = cf }
                6 -> {
                    if (v == 0) { divideError(); return }
                    val q = ax / v; if (q > 0xff) { divideError(); return }
                    ax = ((ax % v) shl 8) or q
                }
                else -> {
                    val d = v.toByte().toInt(); if (d == 0) { divideError(); return }
                    val n = ax.toShort().toInt(); val q = n / d; if (q > 127 || q < -128) { divideError(); return }
                    ax = (((n % d) and 0xff) shl 8) or (q and 0xff)
                }
            }
        } else {
            val v = rm16()
            when (mReg) {
                0, 1 -> logic16(v and fetch16())
                2 -> setRm16(v.inv())
                3 -> { val r = sub16(0, v, 0); setRm16(r); cf = v != 0 }
                4 -> { val r = ax.toLong() * v; ax = (r and 0xffff).toInt(); dx = ((r shr 16) and 0xffff).toInt(); cf = dx != 0; of = cf; szp16(ax) }
                5 -> { val r = ax.toShort().toInt() * v.toShort().toInt(); ax = r and 0xffff; dx = (r shr 16) and 0xffff; cf = r != r.toShort().toInt(); of = cf }
                6 -> {
                    if (v == 0) { divideError(); return }
                    val n = (dx.toLong() shl 16) or ax.toLong(); val q = n / v; if (q > 0xffff) { divideError(); return }
                    ax = q.toInt(); dx = (n % v).toInt()
                }
                else -> {
                    val d = v.toShort().toInt(); if (d == 0) { divideError(); return }
                    val n = (dx shl 16) or ax; val q = n / d; if (q > 32767 || q < -32768) { divideError(); return }
                    ax = q and 0xffff; dx = (n % d) and 0xffff
                }
            }
        }
    }
}
