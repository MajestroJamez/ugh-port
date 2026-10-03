# 3 - Kotlin port

The whole original program ported to Kotlin and verified against the original in lockstep. It is the reference
the remake is checked against, and it is not changed any more.

| Gradle project | Content |
|---|---|
| `core/` (`:core`) | the port: shared address space, VGA model, ported game routines, sound driver, OPL2 synthesizer |
| `oracle/` (`:oracle`) | deterministic 286/VGA/DOS emulator running the original as the reference |
| `desktop/` (`:desktop`) | Windows window (Swing), sound output (Java Sound), Windows package (`:desktop:packageZip`) |
