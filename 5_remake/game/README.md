# UGH! in Unreal Engine 5.8 - the prehistoric diorama

The game logic of `../logic` inside Unreal Engine, shown as a diorama (`docs/visual-concept.md`): the level is a rock
cut exactly in the plane of the play (its collision mask), coloured by the original's drawing of the level, with the
cave behind it, water, fog, a campfire and a wooden box around; the copters, passengers, enemies and bonus items are
plasticine shapes the size of their sprites, a passenger's speech bubble a card with its sprite. The logic runs at
the original's tick (70.086 Hz) whatever the frame rate; every frame is drawn between its last two steps. The
original's sounds and music play on the logic's events (`assets/sound`). No map:
the game mode builds the scene from code (`/Engine/Maps/Entry`); the materials are made by a commandlet, so there
are no binary assets in git.

## Build, test, play, package

Windows PowerShell 5.1, Unreal Engine 5.8 in `C:\Program Files\Epic Games\UE_5.8` (or `$env:UE_ROOT`), the game
data in `assets\` (`.\gradlew.bat :extractor:run`, the sounds `.\gradlew.bat :extractor:sound`; without them the game is
silent and logs it):

| Script | What it does |
|---|---|
| `setup.ps1` | copies the vendor plugins (DLSS + Streamline, FSR with its offscreen patch, about 5 GB, not in git) from the toolchain trial into `Plugins\`; `-From <folder>` elsewhere. Once |
| `build.ps1` | builds the editor (modules UghLogic, UghGame, UghEditor) and makes the materials (commandlet `UghMakeAssets` -> `Content\Generated`); `-NoAssets` only builds |
| `test.ps1` | the automation tests inside the engine without a window and without sound (`UnrealEditor-Cmd -nullrhi -nosound`): the golden replays `Ugh.Replays.*` (they need `.\gradlew.bat :verify:replays`), the menu `Ugh.Menu` and the sounds `Ugh.Sounds.*` (every event's file, the mixer, when what plays); `-Filter` other tests |
| `shot.ps1` | starts a level from the menu by itself without a window, lets the copters hover and saves `Saved\Shots\<mode>-<NN>.png`: `-Level <n>` (from 1), `-Team`, `-At <seconds>` later, `-Commands "<cvar> <value>"` to try a setting |
| `levels.ps1` | the same for every level of both modes in one run, then the contact sheets `Saved\Shots\Levels\levels-1p.png` and `levels-team.png` (and the menu's `menu.png`): does every level look right? `-Levels team:1-81` fewer |
| `play.ps1` | the game in a window |
| `package.ps1` | the game for Windows in `Packaged\Windows` with the data of `assets\` next to it, and a zip without `.pdb` (for your own use: the data is not ours to share). UAT needs `::1` in `NO_PROXY` (the script adds it) |
| `pso.ps1` | the bundled PSO cache: packages, lets the packaged game show the menu and play a calm level, a windy one and the team mode by itself with `-logPSO`, expands the recording with the cook's stable shader keys into `Build\Windows\PipelineCaches` and packages again |

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\build.ps1
```

The menu: Up and Down choose a row (one player or the team, the difficulty, a password), Left and Right change the
mode or the difficulty, letters and digits type a level's password (the original's, `assets/levels.json`), Backspace
deletes, Enter plays, Esc quits. The end of
a game goes back to the menu.

Keys in a game: arrows fly, Right Ctrl or Space fires (pilot 2 of the team mode: W A S D, Left Ctrl - S, not the
original's Z, which is Y on a Czech keyboard); Esc gives the game up, any key goes on from a caption. U switches the
upscaler (DLSS where supported, FSR, TSR), G the DLSS frame generation. Page Up and Page Down change the volume (in the
menu too).

## Modules

| Module | What is in it |
|---|---|
| `Source/UghLogic/` | the logic as a module: `UghLogic.Build.cs` compiles the sources of `../logic/src` where they are (one generated file per source in `Intermediate/UghLogicSources/`, because the logic has files of the same name in different folders and UBT wants unique names). Its C API `ugh_logic.h` is exported (`UGH_LOGIC_API`). In the editor also the replay tests (`Private/Tests/ReplayTests.cpp` with the test pilot and `6_verification`'s replay check) |
| `Source/UghGame/` | the frontend; it uses only the C API |
| `Source/UghEditor/` | the editor's part: the commandlet `UghMakeAssets` that writes the materials of `UghMaterials.h` (clay, rock, water, fire, sprite card) |

The frontend:

| Class | What it does |
|---|---|
| `AUghGameMode` | spawns the stage, the background, the figures, the campfire and the speaker; the menu, then the logic every frame, its view and its sounds; builds the diorama of each level (behind the menu the one the menu would start, dimmed); the keys of the frontend |
| `FUghMenu` | the menu before a game (the mode, the difficulty, a password): its keys and the game it would start (`FUghGameChoice`); the HUD draws it. Test `Ugh.Menu` (`UghMenuTests.cpp`) |
| `FUghPasswords` | the passwords of the levels of both modes (`assets/levels.json`) |
| `FUghSimulation` | the logic at its fixed tick: `Advance(seconds)` runs the steps that are due, keeps the views before and after the last step (`Alpha` between them) and the events of the steps |
| `FUghSounds` | the original's sounds (`assets/sound/*.wav`, read at run time) and which event of the logic plays which (`Cues`, as in the port; the other events had no sound) |
| `FUghMixer` | the sounds that play, mixed into one stream: the music (repeated, a delay, a fade-out) and four effects like the original's channels (a loop until its entity stops it); the volume. Unlike the original the effects do not take the music's voices |
| `FUghSoundPlayer` | when the game plays what: the menu's music, the level's music in the play, the effects of the events, the end of the play (the effects stop, the music fades out with the picture), a lost game's jingle, the ending's music. Tests `Ugh.Sounds.*` (`UghSoundTests.cpp`) |
| `AUghSpeaker` | the mixer's stream through the engine (a procedural sound wave fed a little ahead every frame); silent for the autopilot, with `-nosound` and without the files |
| `FUghKeyboard` | a key event of the engine to the logic (Adapter): pilots' keys, Esc, P, any other key |
| `AUghPlayerController` | passes every key press and release to the game mode (the logic wants raw key events, not input actions) |
| `AUghStage` | the sun, the sky, the fog, the exposure and the camera (fixed, a narrow lens, a little from above); a windy level is a storm (a dim cool sun, a dense grey fog) |
| `FUghRockMesh` | the rock of a level from its collision mask: the cut face, the floors, ceilings and walls one pixel a step, the bumpy back wall; where a campfire fits |
| `FUghLevelArt` | the original's drawing of a level: its tiles (`assets/levels.json`) composed from the sprites into a texture |
| `AUghBackground` | the rock mesh with the level's drawing, the water, the wooden box |
| `AUghCampfire` | logs, a flame and a flickering light (decoration only); in the wind the flame leans and flickers more |
| `AUghFigures` | copters, passengers, enemies, bonus items, raindrops (strokes of clay along their way with the wind): plasticine shapes between two steps (a jump is not interpolated); speech bubbles as sprite cards |
| `FUghSprites` | the sprites of the original: their sizes (`assets/sprites.json`) and pixels (`assets/sprites/NNN.png`) |
| `UghShapes` | where the screen of the original lies in the world (1 px = 10 units, X right, Z up, depth toward the camera negative), the slab of the play, shapes and materials |
| `UghTexture` | pixels as a texture |
| `UghMaterials` | the materials' paths and parameters, shared by the game and the commandlet |
| `FUghShot` | `-UghShot`: the game starts levels from the menu by itself (a player's keys), lets them hover and takes a screenshot of each (`shot.ps1`, `levels.ps1`, `pso.ps1`) |
| `FUghUpscaler` | DLSS / FSR / TSR at 67 % and the frame generation, as tried in the toolchain trial |
| `AUghHud` | the menu with how the last game ended; in a game the status line, the caption, the keys; the upscaler and the volume |
| `UghJson` | reads a JSON file of the extracted data |

## Rules

- The plane of the play is the collision mask, exactly: the rock's cut face is its solid pixels; nothing the
  frontend shows decides anything. The campfire stands on a dry ledge with no pad on it.
- The frontend reads the logic only through `ugh_logic.h`; values it needs (the screen, the copter's body, a full
  tank, the frame rate) come from there, checked against the logic by `static_assert` in `LogicApi.cpp`.
- The original's data stays out of git: the drawing, the sprites and the sounds are read from `assets\` at run
  time.
- FSR is an upscaler only (`r.FidelityFX.FI.Enabled=0`, `OverrideSwapChainDX12=0` in `Config/DefaultEngine.ini`),
  so it does not clash with DLSS frame generation.
