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
| `replay/` | library `ugh_logic_replay`: the semantic state of the game as the fields of `UGR 1` (`StateWriter`, one writer per entity, `FieldRules` when a field is defined, `FieldTable` its value); it only reads the game |
| `keyboard/` | library `ugh_pc_keyboard`: the PC keyboard of the original (Adapter). `KeyFile` reads the `key` records of the data (the logic skips them), `PcKeyboard` turns the scancodes of a replay into the pilots' keys and, before a frame, a menu key when the last scancode changed |
| `tests/` | `verification_tests`: the tests of the keyboard |
| `tools/replay_check/` | `replay_check`: reads a replay tick by tick, feeds its keys and the test pilot's settings to the logic and compares the fields |

The format of the replays and the table of fields: `docs/rewrite-design.md`, chap. 9.

## Patterns

- **Visitor**: `StateWriter` visits the passengers and the enemies of the logic by type (`PassengerVisitor`,
  `EnemyVisitor`); one writer per entity (`PassengerFields`, `EnemyFields` ...) writes its fields.
- **Table of rules**: which fields an entity has in which state is a table at the top of its writer (`FieldRules`):
  a field is written exactly when the original defines it.
- **Table of values**: how to get the value of each field is a table "field -> function" under the rules
  (`FieldTable`: one per entity type and one for what all passengers or all enemies have); `writeFields` puts the two
  together. A new field is one rule and one line of the table.
- **Adapter**: `PcKeyboard` turns the scancodes of the original into the inputs of the logic; the logic never sees a
  scancode.

## Where to change what

| I want to ... | Go to |
|---|---|
| add a field to the replays | the rule and a line of the value table in the entity's writer (`replay/PassengerFields.cpp` ...), and the same field in `4_test_data/verify/.../replay/SemanticProjection.kt` |
| change what a mismatch report shows | `tools/replay_check/ReplayReport.cpp` |
| read a new kind of line of a replay | `tools/replay_check/ReplayFile.cpp` |
| let the test pilot set something new (an I line) | `tools/replay_check/ReplayCheck.cpp` (`intervene`) and `5_remake/logic/testing/TestPilot.hpp` |
| change how the keys of a replay reach the logic | `keyboard/PcKeyboard.cpp` |
