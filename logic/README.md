# UGH! game logic (C++)

The game logic of UGH! (1992), written anew in C++20 without dependencies: it computes every frame exactly as the
original DOS game does (checked against golden replays), but knows nothing of the original's memory. Design:
`re/notes/rewrite-design.md`; the map to the original and the Kotlin port: `re/notes/logic-map.md`.

## Build and test

Windows PowerShell 5.1, with VS Build Tools (MSVC, CMake, Ninja):

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\logic\build.ps1
```

It builds `build\ugh_logic.lib`, the replay check `build\replay_check.exe` and the tests `build\ugh_logic_tests.exe`,
then runs CTest: the tests and one check per golden replay. It needs the game data `assets\sim\ugh-data.ugd`
(`.\gradlew.bat :extractor:run`) and the replays `verify\build\replays\ugr1` (`.\gradlew.bat :verify:replays`).

(The guide to the modules follows in step N8.)
