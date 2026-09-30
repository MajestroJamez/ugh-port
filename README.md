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

## Layout

| Path | Content |
|---|---|
| `extractor/` | reads `UGH.EXE`: Pack-Ice depacker, sprites, palette, levels, pictures, AdLib blocks |
| `re/notes/` | reverse-engineering notes (Czech): executable map, data formats |
| `re/tools/` | Node.js helper scripts used during analysis |
| `re/ghidra-scripts/` | Ghidra headless export scripts |

## Status

1. Executable map (segments, hardware access, game loop) - done
2. Asset extractor - done
3. Deterministic game core verified frame by frame against the original in DOSBox-X - next
4. Complete game 1:1 packaged for Windows
5. Enhancements (new graphics etc.)
