# UGH! in Unreal Engine 5.8 - the prehistoric diorama

The game logic of `../logic` inside Unreal Engine, shown as a diorama (`docs/visual-concept.md`): the level is a rock
cut exactly in the plane of the play (its collision mask), coloured by the original's drawing of the level, with the
cave behind it, water, fog, a campfire and a wooden box around; the copters, passengers, enemies and bonus items are
plasticine shapes the size of their sprites, a passenger's speech bubble a card with its sprite. The logic runs at
the original's tick (70.086 Hz) whatever the frame rate; every frame is drawn between its last two steps. No map:
the game mode builds the scene from code (`/Engine/Maps/Entry`); the materials are made by a commandlet, so there
are no binary assets in git.

## Build, test, play, package

Windows PowerShell 5.1, Unreal Engine 5.8 in `C:\Program Files\Epic Games\UE_5.8` (or `$env:UE_ROOT`), the game
data in `assets\` (`.\gradlew.bat :extractor:run`):

| Script | What it does |
|---|---|
| `setup.ps1` | copies the vendor plugins (DLSS + Streamline, FSR with its offscreen patch, about 5 GB, not in git) from the toolchain trial into `Plugins\`; `-From <folder>` elsewhere. Once |
| `build.ps1` | builds the editor (modules UghLogic, UghGame, UghEditor) and makes the materials (commandlet `UghMakeAssets` -> `Content\Generated`); `-NoAssets` only builds |
| `test.ps1` | the golden replays inside the engine without a window (automation tests `Ugh.Replays.*`, `UnrealEditor-Cmd -nullrhi`); needs the replays (`.\gradlew.bat :verify:replays`) |
| `shot.ps1` | plays level 1 by itself without a window and saves `Saved\Shots\level1.png`; `-At <seconds>` later, `-Commands "<cvar> <value>"` to try a setting |
| `play.ps1` | the game in a window |
| `package.ps1` | the game for Windows in `Packaged\Windows` with the data of `assets\` next to it, and a zip without `.pdb` (for your own use: the data is not ours to share). UAT needs `::1` in `NO_PROXY` (the script adds it) |
| `pso.ps1` | the bundled PSO cache: packages, lets the packaged game play level 1 by itself with `-logPSO`, expands the recording with the cook's stable shader keys into `Build\Windows\PipelineCaches` and packages again |

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
| `Source/UghEditor/` | the editor's part: the commandlet `UghMakeAssets` that writes the materials of `UghMaterials.h` (clay, rock, water, fire, sprite card) |

The frontend:

| Class | What it does |
|---|---|
| `AUghGameMode` | spawns the stage, the background, the figures and the campfire, runs the logic every frame and shows it, builds the diorama of each level; the keys of the frontend |
| `FUghSimulation` | the logic at its fixed tick: `Advance(seconds)` runs the steps that are due, keeps the views before and after the last step (`Alpha` between them) |
| `FUghKeyboard` | a key event of the engine to the logic (Adapter): pilots' keys, Esc, P, any other key |
| `AUghPlayerController` | passes every key press and release to the game mode (the logic wants raw key events, not input actions) |
| `AUghStage` | the sun, the sky, the fog, the exposure and the camera (fixed, a narrow lens, a little from above) |
| `FUghRockMesh` | the rock of a level from its collision mask: the cut face, the floors, ceilings and walls one pixel a step, the bumpy back wall; where a campfire fits |
| `FUghLevelArt` | the original's drawing of a level: its tiles (`assets/levels.json`) composed from the sprites into a texture |
| `AUghBackground` | the rock mesh with the level's drawing, the water, the wooden box |
| `AUghCampfire` | logs, a flame and a flickering light (decoration only) |
| `AUghFigures` | copters, passengers, enemies, bonus items, raindrops: plasticine shapes between two steps (a jump is not interpolated); speech bubbles as sprite cards |
| `FUghSprites` | the sprites of the original: their sizes (`assets/sprites.json`) and pixels (`assets/sprites/NNN.png`) |
| `UghShapes` | where the screen of the original lies in the world (1 px = 10 units, X right, Z up, depth toward the camera negative), the slab of the play, shapes and materials |
| `UghTexture` | pixels as a texture |
| `UghMaterials` | the materials' paths and parameters, shared by the game and the commandlet |
| `FUghShot` | `-UghShot`: the game plays level 1 by itself (hovering) and takes one screenshot |
| `FUghUpscaler` | DLSS / FSR / TSR at 67 % and the frame generation, as tried in the toolchain trial |
| `AUghHud` | the status line, the caption, the end of the game, the keys |

## Rules

- The plane of the play is the collision mask, exactly: the rock's cut face is its solid pixels; nothing the
  frontend shows decides anything. The campfire stands on a dry ledge with no pad on it.
- The frontend reads the logic only through `ugh_logic.h`; values it needs (the screen, the copter's body, a full
  tank, the frame rate) come from there, checked against the logic by `static_assert` in `LogicApi.cpp`.
- The original's data stays out of git: the drawing and the sprites are read from `assets\` at run time.
- FSR is an upscaler only (`r.FidelityFX.FI.Enabled=0`, `OverrideSwapChainDX12=0` in `Config/DefaultEngine.ini`),
  so it does not clash with DLSS frame generation.
