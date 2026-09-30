// Disassembles and creates functions at the addresses listed in a text file (one "seg:off" per line,
// '#' comments allowed), then lets auto-analysis follow the new flows.
// Usage (headless): -postScript DisassembleAt.java <addresses.txt>
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;

import java.nio.file.Files;
import java.nio.file.Path;

public class DisassembleAt extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        int created = 0, disassembled = 0;
        for (String line : Files.readAllLines(Path.of(args[0]))) {
            String s = line.replaceAll("#.*", "").trim();
            if (s.isEmpty()) continue;
            if (s.startsWith("clear ")) {
                // "clear seg:off seg:off" removes mis-disassembled instructions in the range (inclusive)
                String[] p = s.split("\\s+");
                Address from = currentProgram.getAddressFactory().getAddress(p[1]);
                Address to = currentProgram.getAddressFactory().getAddress(p[2]);
                for (var f : currentProgram.getFunctionManager().getFunctions(new AddressSet(from, to), true)) {
                    currentProgram.getFunctionManager().removeFunction(f.getEntryPoint());
                }
                clearListing(from, to);
                println("cleared " + from + " .. " + to);
                continue;
            }
            Address a = currentProgram.getAddressFactory().getAddress(s);
            if (a == null) { println("bad address " + s); continue; }
            if (getInstructionAt(a) == null) {
                DisassembleCommand cmd = new DisassembleCommand(a, null, true);
                if (cmd.applyTo(currentProgram, monitor)) disassembled++;
                else println("disassembly failed at " + s + ": " + cmd.getStatusMsg());
            }
            if (getFunctionAt(a) == null) {
                CreateFunctionCmd fc = new CreateFunctionCmd(a);
                if (fc.applyTo(currentProgram, monitor)) created++;
                else println("function failed at " + s + ": " + fc.getStatusMsg());
            }
        }
        analyzeChanges(currentProgram);
        println("disassembled " + disassembled + ", functions created " + created);
    }
}
