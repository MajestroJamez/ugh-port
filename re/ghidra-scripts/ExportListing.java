// Exports a plain-text disassembly of the real code segments (CODE_0..CODE_4)
// plus a table of which functions reference which memory block.
// Usage (headless): -postScript ExportListing.java <outDir>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.Reference;

import java.io.File;
import java.io.PrintWriter;
import java.util.*;

public class ExportListing extends GhidraScript {

    private static final Set<String> CODE = Set.of("CODE_0", "CODE_1", "CODE_2", "CODE_3", "CODE_4");

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();
        Listing listing = currentProgram.getListing();
        FunctionManager fm = currentProgram.getFunctionManager();

        // segment value -> block name, for immediate "MOV reg,seg" style references
        Map<Long, String> segToBlock = new HashMap<>();
        for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            if (b.getStart().getAddressSpace().getName().equals("HEADER")) continue;
            long seg = b.getStart().getOffset() >> 4;
            segToBlock.put(seg, b.getName());
        }

        Map<String, TreeSet<String>> blockUsers = new TreeMap<>();
        try (PrintWriter pw = new PrintWriter(new File(outDir, "listing.asm"), "UTF-8")) {
            for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
                if (!CODE.contains(b.getName())) continue;
                pw.println(";; ================= " + b.getName() + " =================");
                InstructionIterator it = listing.getInstructions(b.getStart(), true);
                while (it.hasNext()) {
                    Instruction ins = it.next();
                    if (!b.contains(ins.getAddress())) break;
                    Function f = fm.getFunctionAt(ins.getAddress());
                    if (f != null) pw.println("\n;; ----- " + f.getName() + " -----");
                    Function owner = fm.getFunctionContaining(ins.getAddress());
                    String ownerName = owner == null ? "?" : owner.getName();
                    StringBuilder extra = new StringBuilder();
                    for (Reference r : ins.getReferencesFrom()) {
                        Address to = r.getToAddress();
                        MemoryBlock tb = currentProgram.getMemory().getBlock(to);
                        if (tb != null && !tb.getName().equals(b.getName())) {
                            blockUsers.computeIfAbsent(tb.getName(), k -> new TreeSet<>()).add(ownerName);
                        }
                        Function tf = fm.getFunctionAt(to);
                        extra.append(" -> ").append(tf != null ? tf.getName() : to.toString());
                    }
                    // immediate segment loads (MOV AX,0x2e1f etc.)
                    for (int i = 0; i < ins.getNumOperands(); i++) {
                        for (Object o : ins.getOpObjects(i)) {
                            if (o instanceof Scalar s) {
                                String bn = segToBlock.get(s.getUnsignedValue());
                                if (bn != null && s.getUnsignedValue() >= 0x1a67) {
                                    blockUsers.computeIfAbsent(bn, k -> new TreeSet<>()).add(ownerName);
                                    extra.append(" ;seg ").append(bn);
                                }
                            }
                        }
                    }
                    StringBuilder hex = new StringBuilder();
                    for (byte x : ins.getBytes()) hex.append(String.format("%02x", x & 0xff));
                    pw.printf("%s  %-40s%s  ;; len=%d bytes=%s%n", ins.getAddress(), ins, extra, ins.getLength(), hex);
                }
            }
        }
        try (PrintWriter pw = new PrintWriter(new File(outDir, "block-users.txt"), "UTF-8")) {
            for (var e : blockUsers.entrySet()) pw.println(e.getKey() + ": " + e.getValue());
        }
        println("Listing done");
    }
}
