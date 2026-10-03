# UGH! in Unreal Engine 5.8 - grey boxes

The game logic of `../logic` inside Unreal Engine: the level from its collision mask as boxes, the copters,
passengers, enemies and bonus items as boxes the size of their sprites, the text of the original on the HUD. The
logic runs at the original's tick (70.086 Hz) whatever the frame rate; every frame is drawn between its last two
steps. No map and no assets of our own: the game mode builds the scene from code (`/Engine/Maps/Entry`).

## Build, test, play

Windows PowerShell 5.1, Unreal Engine 5.8 in `C:\Program Files\Epic Games\UE_5.8` (or `$env:UE_ROOT`), the game
data in `assets\` (`.\gradlew.bat :extractor:run`):

| Script | What it does |
|---|---|
| `setup.ps1` | copies the vendor plugins (DLSS + Streamline, FSR with its offscreen patch, about 5 GB, not in git) from the toolchain trial into `Plugins\`; `-From <folder>` elsewhere. Once |
| `build.ps1` | builds the editor with both modules |
| `test.ps1` | the golden replays inside the engine without a window (automation tests `Ugh.Replays.*`, `UnrealEditor-Cmd -nullrhi`); needs the replays (`.\gradlew.bat :verify:replays`) |
| `shot.ps1` | plays level 1 by itself without a window and saves `Saved\Shots\level1.png` |
| `play.ps1` | the game in a window |

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\build.ps1
```

Keys: arrows fly, Right Ctrl or Space fires (pilot 2 of the team mode: W A S D, Left Ctrl - S, not the original's
Z, which is Y on a Czech keyboard); Esc gives the game up, any key goes on from a caption. U switches the upscaler
(DLSS where supported, FSR, TSR), G the DLSS frame generation. When the game is over: Enter a new game, Esc quit.

## Modules

| Module | What is in it |
|---|---|
| `Source/UghLogic/` | the logic as a module: `UghLogic.Build.cs` compiles the sources of `../logic/src` where they are (one generated file per source in `Intermediate/UghLogicSources/`, because the logic has files of the same name in different folders and UBT wants unique names). Its C API `ugh_logic.h` is exported (`UGH_LOGIC_API`). In the editor also the replay tests (`Private/Tests/ReplayTests.cpp` with the test pilot and `6_verification`'s replay check) |
| `Source/UghGame/` | the frontend; it uses only the C API |

The frontend:

| Class | What it does |
|---|---|
| `AUghGameMode` | builds the stage (light, camera, background, figures), runs the logic every frame and shows it; the keys of the frontend; `-UghShot` |
| `FUghSimulation` | the logic at its fixed tick: `Advance(seconds)` runs the steps that are due, keeps the views before and after the last step (`Alpha` between them) |
| `FUghKeyboard` | a key event of the engine to the logic (Adapter): pilots' keys, Esc, P, any other key |
| `AUghPlayerController` | passes every key press and release to the game mode (the logic wants raw key events, not input actions) |
| `AUghBackground` | the level being played: the collision mask merged into boxes, the pads with their numbers, the water, a wall behind |
| `AUghFigures` | copters, passengers (with bubbles), enemies, bonus items, raindrops: boxes between two steps (a jump is not interpolated) |
| `UghShapes` | where the screen of the original lies in the world (1 px = 10 units, X right, Z up, Y depth) and boxes of one colour |
| `FUghSpriteSizes` | the size of every sprite (`assets/sprites.json`) |
| `FUghUpscaler` | DLSS / FSR / TSR at 67 % and the frame generation, as tried in the toolchain trial |
| `AUghHud` | the status line, the caption, the end of the game, the keys |

## Rules

- The plane of the play is the collision mask, exactly: the boxes of the level are its solid pixels; nothing the
  frontend shows decides anything.
- The frontend reads the logic only through `ugh_logic.h`; sizes it needs (the screen, the copter's body, a full
  tank) come from there, checked against the logic by `static_assert` in `LogicApi.cpp`.
- FSR is an upscaler only (`r.FidelityFX.FI.Enabled=0`, `OverrideSwapChainDX12=0` in `Config/DefaultEngine.ini`),
  so it does not clash with DLSS frame generation.
- The sun casts no shadows: on the stretched boxes they fall as long streaks. The visual direction (step 11) brings
  real meshes and its own light.
