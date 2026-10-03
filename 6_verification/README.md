# 6 - Verification

The C++ game logic of `5_remake/logic` against the golden replays of `4_test_data/verify`: the same keys in, the
same state out, every field of every frame.

## Build and test

Windows PowerShell 5.1, with VS Build Tools (MSVC, CMake, Ninja):

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\6_verification\build.ps1
```

It builds the logic (`add_subdirectory(../5_remake/logic)`), the fields of the replays, the keyboard and the replay
check, then runs CTest: the unit tests of the logic, the tests of the keyboard and one check per golden replay (161).
`-NoTest` only builds. It needs the game data `assets\logic\ugh-data.ugd` (`.\gradlew.bat :extractor:run`) and the replays
`4_test_data\verify\build\replays` (`.\gradlew.bat :verify:replays`). One replay by hand, from this folder:

```
build\replay_check.exe ..\assets\logic\ugh-data.ugd ..\4_test_data\verify\build\replays\1p-L01-cheat.ugr
```

(`--continue` counts all mismatches instead of stopping at the first.)

## What is in it

| Folder | Content |
|---|---|
| `replay/` | library `ugh_logic_replay`: the semantic state of the game as the fields of `UGR 1` (`StateWriter`, one writer per entity, `FieldRules` when a field is defined); it only reads the game |
| `keyboard/` | library `ugh_pc_keyboard`: the PC keyboard of the original (Adapter). `KeyFile` reads the `key` records of the data (the logic skips them), `PcKeyboard` turns the scancodes of a replay into the pilots' keys and, before a frame, a menu key when the last scancode changed |
| `tests/` | `verification_tests`: the tests of the keyboard |
| `tools/replay_check/` | `replay_check`: reads a replay tick by tick, feeds its keys and the test pilot's settings to the logic and compares the fields |

The format of the replays and the table of fields: `docs/rewrite-design.md`, chap. 9.
