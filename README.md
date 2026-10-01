# ugh-port

A faithful re-implementation of the 1994 DOS game **UGH!** in Kotlin, aiming at identical behaviour
(levels, difficulty, physics, 2-player team mode) on modern Windows, with optional enhancements later.

## No game data included

This repository contains **only original code and notes**. It does not contain the game executable, graphics,
levels, music or texts. UGH! is © Play Byte / Bones Park Software Artistic; "abandonware" is not a licence.

To use the port you need your own copy of the original `UGH.EXE` (the extractor checks its SHA-256,
`ef93d2cd5eb558f6a7d0007e0109f2e9ee952dc125389d7a6256646087636d7c`). The extractor reads the data straight
from it into the local, git-ignored `assets/` directory.

## Build

Requirements: JDK 25 (the Gradle wrapper downloads everything else).

```powershell
# put the original game into OLD\UGH.EXE, then:
.\gradlew.bat :extractor:test
.\gradlew.bat :extractor:run
```

## Play

The desktop window runs the port (game code, graphics, AdLib music and effects through its own OPL2
synthesizer):

```powershell
.\gradlew.bat :desktop:run
```

For comparison, `--original` runs the original `UGH.EXE` in the project's deterministic emulator instead
(`:oracle`, no sound):

```powershell
.\gradlew.bat :desktop:run --args="--original"
```

### Windows package

```powershell
.\gradlew.bat :desktop:packageZip
```

builds `desktop\build\jpackage\UGH-port\` (`UGH-port.exe` with its own Java runtime, no Java needed on the
target machine) and `desktop\build\distributions\UGH-port-windows.zip`. The original game is not part of it:
put `UGH.EXE` next to `UGH-port.exe`, or start it and pick `UGH.EXE` once (the path is remembered in
`%APPDATA%\ugh-port`).

Player 1: arrow keys (up = pedal), player 2: W / Z / A / S. F1 starts, F3 difficulty, F4 one player / team.
The high score table is kept in `%APPDATA%\ugh-port`.

## Layout

| Path | Content |
|---|---|
| `extractor/` | reads `UGH.EXE`: Pack-Ice depacker, sprites, palette, levels, pictures, AdLib blocks |
| `core/` | the port: shared address space, VGA model, ported game routines, sound driver, OPL2 synthesizer |
| `oracle/` | deterministic 286/VGA/DOS emulator running the original as the reference |
| `verify/` | differential tests: every ported routine against the original, byte for byte |
| `desktop/` | Windows window (Swing) and sound output (Java Sound) |
| `re/notes/` | reverse-engineering notes (Czech): executable map, data formats |
| `re/tools/` | Node.js helper scripts used during analysis |
| `re/ghidra-scripts/` | Ghidra headless export scripts |

## Status

1. Executable map (segments, hardware access, game loop) - done
2. Asset extractor - done
3. Deterministic game core verified against the original - done: the whole program (game logic, drawing,
   all screens, AdLib sound driver) ported and verified in lockstep with the original; own OPL2 synthesizer;
   the desktop window runs the port
4. Complete game 1:1 packaged for Windows - done (`:desktop:packageZip`)
5. Enhancements (new graphics etc.)
