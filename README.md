# ugh-port

A faithful re-implementation of the 1994 DOS game **UGH!** in Kotlin, aiming at identical behaviour
(levels, difficulty, physics, 2-player team mode) on modern Windows, with optional enhancements later.

## No game data included

This repository contains **only original code and notes**. It does not contain the game executable, graphics,
levels, music or texts. UGH! is © Play Byte / Bones Park Software Artistic; "abandonware" is not a licence.

To use the port you need your own copy of the original `UGH.EXE` (the extractor checks its SHA-256,
`ef93d2cd5eb558f6a7d0007e0109f2e9ee952dc125389d7a6256646087636d7c`). The extractor reads the data straight
from it into the local, git-ignored `assets/` directory.

The copy this port was made from was downloaded from <https://mujsoubor.cz/stare-hry/ugh> (a Czech site
with old DOS games). Check the SHA-256 above: the port only works with exactly that version.

## Build

Requirements: JDK 25 (the Gradle wrapper downloads everything else).

```powershell
# put the original game into 1_original\UGH.EXE, then:
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

builds `3_kotlin_port\desktop\build\jpackage\UGH-port\` (`UGH-port.exe` with its own Java runtime, no Java needed
on the target machine) and `3_kotlin_port\desktop\build\distributions\UGH-port-windows.zip`. The original game is not part of it:
put `UGH.EXE` next to `UGH-port.exe`, or start it and pick `UGH.EXE` once (the path is remembered in
`%APPDATA%\ugh-port`).

Releases are built the same way by GitHub Actions (`.github/workflows/release.yml`) when a version tag `v*` is
pushed; the release text is `.github/release-notes.md`.

Player 1: arrow keys (up = pedal), player 2: W / Z / A / S. F1 starts, F3 difficulty, F4 one player / team.
The high score table is kept in `%APPDATA%\ugh-port`.

## Layout: the steps of the project

The folders are numbered in the order the project was made, which is also the order to read it in; a later step
uses only earlier ones. Each numbered folder has a short `README.md`.

| Folder | Step |
|---|---|
| `1_original/` | your own `UGH.EXE` (never committed) |
| `2_reverse_engineering/` | the analysis of the original: notes (Czech, `notes/`), Node.js tools (`tools/`), Ghidra scripts (`ghidra-scripts/`) |
| `3_kotlin_port/` | the Kotlin port: `core/` (the ported program, OPL2 synthesizer), `oracle/` (deterministic 286/VGA/DOS emulator running the original), `desktop/` (Windows window and sound) |
| `4_test_data/` | `extractor/` (reads `UGH.EXE` into `assets/`, also the data of the C++ logic), `verify/` (the port against the original in lockstep, golden replays UGR 1) |
| `5_remake/` | the remake: `logic/` (C++ game logic, library and unit tests), `game/` (Unreal Engine project, step 10) |
| `6_verification/` | the C++ logic against the golden replays, field by field (`build.ps1`) |
| `docs/` | the plan of the project (`plan.md`), the design and the map of the C++ logic |
| `assets/` | data extracted from `UGH.EXE` (never committed) |

The Gradle wrapper and build scripts stay in the root: `.\gradlew.bat :extractor:run`, `:verify:replays` or
`:desktop:run` work from there.

## Status

1. Executable map (segments, hardware access, game loop) - done
2. Asset extractor - done
3. Deterministic game core verified against the original - done: the whole program (game logic, drawing,
   all screens, AdLib sound driver) ported and verified in lockstep with the original; own OPL2 synthesizer;
   the desktop window runs the port
4. Complete game 1:1 packaged for Windows - done (`:desktop:packageZip`)
5. Enhancements (new graphics etc.)
