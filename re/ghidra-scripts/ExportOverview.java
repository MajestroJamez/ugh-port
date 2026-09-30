// Exports an overview of the analyzed program: memory blocks, functions,
// INT / IN / OUT instructions and a full decompilation.
// Usage (headless): -postScript ExportOverview.java <outDir>
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;

import java.io.File;
import java.io.PrintWriter;

public class ExportOverview extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();

        try (PrintWriter pw = new PrintWriter(new File(outDir, "blocks.txt"), "UTF-8")) {
            for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
                pw.printf("%-12s %s - %s  size=%d  %s%s%s init=%b%n", b.getName(), b.getStart(), b.getEnd(), b.getSize(),
                        b.isRead() ? "r" : "-", b.isWrite() ? "w" : "-", b.isExecute() ? "x" : "-", b.isInitialized());
            }
        }

        FunctionManager fm = currentProgram.getFunctionManager();
        try (PrintWriter pw = new PrintWriter(new File(outDir, "functions.txt"), "UTF-8")) {
            for (Function f : fm.getFunctions(true)) {
                int callers = f.getCallingFunctions(monitor).size();
                int callees = f.getCalledFunctions(monitor).size();
                pw.printf("%s  %-28s size=%5d callers=%3d callees=%3d%n", f.getEntryPoint(), f.getName(),
                        f.getBody().getNumAddresses(), callers, callees);
            }
        }

        Listing listing = currentProgram.getListing();
        try (PrintWriter pw = new PrintWriter(new File(outDir, "hw.txt"), "UTF-8")) {
            InstructionIterator it = listing.getInstructions(true);
            Instruction prev2 = null, prev1 = null;
            while (it.hasNext()) {
                Instruction ins = it.next();
                String m = ins.getMnemonicString();
                if (m.equals("INT") || m.equals("IN") || m.equals("OUT") || m.startsWith("INS") || m.startsWith("OUTS")) {
                    Function f = fm.getFunctionContaining(ins.getAddress());
                    pw.printf("%s %-24s %-28s | %s | %s%n", ins.getAddress(), f == null ? "?" : f.getName(), ins,
                            prev2 == null ? "" : prev2.toString(), prev1 == null ? "" : prev1.toString());
                }
                prev2 = prev1;
                prev1 = ins;
            }
        }

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        try (PrintWriter pw = new PrintWriter(new File(outDir, "decomp.c"), "UTF-8")) {
            for (Function f : fm.getFunctions(true)) {
                if (monitor.isCancelled()) break;
                DecompileResults r = di.decompileFunction(f, 60, monitor);
                pw.printf("// ===== %s @ %s =====%n", f.getName(), f.getEntryPoint());
                if (r != null && r.decompileCompleted()) {
                    pw.println(r.getDecompiledFunction().getC());
                } else {
                    pw.println("// decompile failed: " + (r == null ? "null" : r.getErrorMessage()));
                }
            }
        }
        di.dispose();
        println("Export done: " + outDir.getAbsolutePath());
    }
}
