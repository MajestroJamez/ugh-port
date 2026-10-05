# UGH! in Unreal Engine 5.8 - the prehistoric diorama

The game logic of `../logic` inside Unreal Engine, shown as a diorama (`docs/visual-concept.md`): the level is a
cliff cut exactly in the plane of the play (its edge there is the collision mask), a smooth rock with rounded edges,
its cave reaching back to a far back wall dressed with scanned grey cliffs and hanging roots, stalactites and
fallen rocks, in scanned rock, soil, grass and moss steered by the original's drawing (its face weathered grey karst
limestone of fractured blocks, moss and lichen, grass hanging over its edges, wet at the water), the doors of the
drawing cave entrances (an arch of rock around a dark passage going deep in, where the passengers come out), the cliff going on beyond the screen in a sea
(waves, foam at the rock, rings around swimmers, caustics, rising with the logic's water) under an HDR sky in the
mood of the level (a day, a golden evening, a dusk, a night lit by the campfires, a storm with rain blown by the wind), a stone age jungle on its ledges (meadows of grass and flowers, palms with big leaves, bushes, ferns, jungle plants, rocks, stumps, bones, totems, a hut, campfires of scanned stones and charred logs glowing with embers, their flames baked from a simulated fire, sparks and a wisp of smoke, their flickering light making the shadows on the rock dance), torches wedged into the rock beside the cave entrances and on the back wall, and ivy
from its ceilings and down its back wall, all behind the figures, here and there a spring pouring out of the back wall, its stream running
across a ledge in a channel under a footbridge of logs and over the edge into the sea as a waterfall with spray at its foot, a weathered board on a post at every pad with its
number carved in tally marks as the original's boards show it; the scanned assets come from Epic's Electric Dreams sample where it was copied (`electric-dreams.ps1`),
else free ones; the copters are stone age pedal
copters of bamboo, wicker, leather, bone and stone (made by Blender scripts) with a stone age man sitting and
pedalling them and their passengers sitting behind him or hanging below; the passengers are stone age people (a man, a
woman, an old man) as big as their sprites, walking, waving, waiting, swimming and falling as their sprites say (the
standing passenger a scanned mossy rock with wet eyes under lids of rock, sitting smaller in a cabin) - photoreal
MetaHumans made with MetaHuman Creator where they were made (`metahumans.ps1`; hair and beards of hair cards,
loincloths of big leaves), else a cartoon caveman; the enemies a pteranodon with baked skin and veined wings, Jan's
triceratops refined (its own animations), Jan's T-rex lying asleep on its belly, breathing in deeply and snorting dust
out of its nostrils, an old hornbeam of scanned bark and crowns with a face carved into its bark - where Jan's models
and the scans are there (made of them by Blender scripts), else cartoon ones -, the bonus items fruits and a stone
tablet (all made by Blender scripts, clay shapes the size of their sprites without them), a passenger's speech bubble drawn sharp (the board of the pad it wants
to go to, or a question mark) with one tail pointing at it. The whole cliff is the face of a big sea stack of karst
limestone standing in the open sea, a jungle on its top: while the caption of a level shows, the camera flies to it
low over the waves like a drone and brakes to the game's view. The logic runs at the original's tick (70.086 Hz)
whatever the frame rate; every frame is drawn
between its last two steps. The original's sounds and music play on the logic's events (`assets/sound`). No map:
the game mode builds the scene from code (`/Engine/Maps/Entry`); the materials are made by a commandlet and the 3D
assets (free models, textures and skies of `Assets.json`) are downloaded and imported by scripts, the Electric Dreams
sample's copied by a script, so there are no binary assets in git.

## Build, test, play, package

Windows PowerShell 5.1, Unreal Engine 5.8 in `C:\Program Files\Epic Games\UE_5.8` (or `$env:UE_ROOT`), the game
data in `assets\` (`.\gradlew.bat :extractor:run`, the sounds `.\gradlew.bat :extractor:sound`; without them the game is
silent and logs it):

| Script | What it does |
|---|---|
| `setup.ps1` | copies the vendor plugins (DLSS + Streamline, FSR with its offscreen patch, about 5 GB, not in git) from the toolchain trial into `Plugins\`; `-From <folder>` elsewhere. Once |
| `fetch-assets.ps1` | downloads the 3D assets of `Assets.json` to `assets\3d` (see 3D assets below): only what is missing, each file checked by its size and hash, the whole under the budget of 100 GB, archives extracted, Blender scripts run (a generated asset is made by its script, again when a script of `Blender/` is newer than it); a local asset (Jan's T-rex and triceratops, the scans of `electric-dreams.ps1`) is never downloaded: without its files it is absent, and so is what is made of it, which is no failure (the game shows the older model); a table of the assets at the end. `-Only <ids>` some, `-Verify` hashes the files already there too |
| `electric-dreams.ps1` | copies the scanned assets of Epic's Electric Dreams sample the game uses (`UghElectricDreams.h`: cliffs, roots, palms, jungle plants, ferns, ivy, grass, stones, rock surfaces; about 4 GB) with everything they need into `Content\External\ElectricDreams` (commandlet `UghCopyElectricDreams`, see 3D assets below), then exports the scans the Blender scripts make the tree and the stone passenger of (`UghElectricDreams::ForBlender`: a hornbeam sapling, a mossy rock) to `assets\3d\electricdreams` as OBJ and PNG (commandlet `UghExportElectricDreams`; `fetch-assets.ps1` and `build.ps1` after it); `-Source <folder>` the sample's project (default `..\Unreal Projects\ElectricDreamsEnv` next to the repository). After `build.ps1`; again after a change of `UghElectricDreams.h` |
| `metahumans.ps1` | makes the pilot and the passengers with MetaHuman Creator in the editor (without a window) into `Content\External\MetaHumans`: `Python\metahumans.py` (presets of Creator's optional content with wild hair and beards, without outfits, their face rig and skin textures from Epic's cloud - the editor logged in to an Epic account -, assembled; only the missing ones, `-All` all again) and `Python\metahuman_actions.py` (an animation per action for each); see The MetaHumans below. After `build.ps1` |
| `build.ps1` | builds the editor (modules UghLogic, UghGame, UghEditor), makes the materials (commandlet `UghMakeAssets` -> `Content\Generated`), bakes the flames of the campfires and torches (commandlet `UghMakeFlames`: a fire simulated as a gas offline, `FUghFireSim`, laid out as looping flipbooks -> `Content\Generated`, a look at them in `Saved\Flames`; about 2 minutes) and imports the downloaded 3D assets (commandlet `UghImportAssets` -> `Content\Imported`); `-NoAssets` only builds, `-ForceImport` imports every asset again |
| `test.ps1` | the automation tests inside the engine without a window and without sound (`UnrealEditor-Cmd -nullrhi -nosound`): the golden replays `Ugh.Replays.*` (they need `.\gradlew.bat :verify:replays`), the menu `Ugh.Menu`, the sounds `Ugh.Sounds.*` (every event's file, the mixer, when what plays) the decorations `Ugh.Scenery` (every level rich - at least 150 of 6 kinds, 100 tufts of grass and flowers, a palm -, the same every time, each on the rock at its own depth with its box in the air, none on the screen nearer than the slab of the play, where the figures reach, where a rotor or a flyer's wing sweeps, where a copter lands on a pad, in front of a pad's board (`UghPadSigns`) or of a cave entrance's passage - but ground cover in front of its arch; nothing in a campfire; at most 4 torches a level, each wedged into a wall behind the rotors' sweep, beside an entrance in most levels with one), the rock `Ugh.Rock` (in every level, with its cave entrances, the rock's mesh cut in the slab of the play is the collision mask: every pixel's centre on its side, every other point at most half a pixel off - the cut leaves the corners of the pixel steps), the rock's dressing `Ugh.Dressing` (every level has scanned cliffs, the same every time, each coming out of the cave's back wall, behind every figure's sweep and behind the middles of the decorations in front of it, none seen in a cave entrance's passage), the cave entrances `Ugh.Portals` (one at each door of every level's drawing, on the mask's floor, open from in front of its arch through its mouth with room for a passenger coming out, rock above and beside the opening, a floor in the passage and an end deep in; behind the sweep of rotors and wings), the copters `Ugh.Copter.*` (the rotor turns as fast as its sprites change; the imported model fills the copter's body and stays in it, the stone passenger is as big as its sprite and sits smaller on the passenger's seat in the body) and the figures `Ugh.Figures.*` (every sprite an entity of the data shows is an action of a model, `FUghFigureActions`; the actions that follow the sprites' frames go on smoothly; the enemies' models - whichever are shown - are as big as their sprites, the T-rex has the nostrils and the head its snort needs, the bonus items are there), the pads' boards `Ugh.Signs` (every pad of every level has exactly one board with its number, the original's board where it draws one - it says the same but one slip of the drawing -, on the screen over its pad, on the rock, clear of the other boards; every board the original draws stands), the bubbles `Ugh.Bubbles` (each bubble of the data shows its pad's board or a question; one tail, on the left where the original draws the bubble right of its passenger, on the right where that would leave the screen, its tip at the passenger's head), the people `Ugh.Figures.People` (the MetaHumans' actions, posed by the engine as the game shows them, put them where the game wants them: on the ground and in the slab of the play, on the seat, the feet on the copter's pedals and the hands on its handles, the hands on the rope, the head at the water's surface; their leaves cover their hips from the belt to the thighs, the woman's her chest too), the water `Ugh.Water.Level` (in a level whose water rises the sea's surface is the logic's water level between two steps in every frame, the sea reaches beyond the screen and towards the camera, the open sea of the flight all around), the rain `Ugh.Rain.Wind` (in a level with wind to the left and one to the right the rain's streaks and the logic's raindrops fall the way the logic's raindrops go) the moods `Ugh.Mood` (the same level the same mood, a windy level a storm, every mood of a calm day among the levels), the flight at a level's start `Ugh.Intro` (it ends exactly in the game's camera, starts far away low over the sea, stays above the water and away from the stone, never past the game's camera, smooth, banking a little, stopping softly, its lens narrowing; a key hurries it to its end before the play can begin) the springs `Ugh.Streams` (where there is room in a level - its rock under the waterfall all the way down under the water, away from pads, boards and cave entrances -, the same every time, the bridge's logs under the ledge's surface and its handrail behind the slab of the play no taller than ground cover, the waterfall in front of the face coming nearer as it falls, the spring on the back wall, the water over its bed; in at least 10 levels) the bursts of the events `Ugh.Effects.*` (every event of the logic has a burst - the table below -, a loop of its sound a loop of its burst, each short and not hiding a figure; an event given to the sounds and the effects together plays its sound and shows its burst where it happens - at its entity, where it was last seen, its player's copter, the water -; a copter coming down onto a pad and a bonus item landing raise dust, a copter settling onto a pad not) and the sea stack `Ugh.Stack` (the level's rock sits in its hollow - the rock's open border and the cave's shroud inside the stone -, nothing of it or of its jungle seen by the game's camera, its plants on it, the same every time); `-Filter` other tests |
| `shot.ps1` | starts a level from the menu by itself without a window, lets the copters hover (the log says the frame rate meanwhile) and saves `Saved\Shots\<mode>-<NN>.png`: `-Level <n>` (from 1), `-Team`, `-At <seconds>` later, `-Commands "<cvar> <value>"` to try a setting, `-Cargo <look>` the copters shown with a passenger of that look in the cabin (1 .. 4: 4 the stone, smaller; `-Hanging` below instead; only the picture, the autopilot never picks one up), `-Bubbles` every passenger shown with a speech bubble (each the next of the data's; only the picture), `-CloseUp` framing the copters, `-Frame <left>,<top>,<width>,<height>` that part of the screen (pixels: a look at the figures; the name then ends in `-cargo<look>` or `-hanging<look>`, `-bubbles`, `-closeup`, `-frame<left>_<top>`), `-Look campfire|torch` the first campfire (torch) of the level close up (with `-Frame` that part from its middle; the name ends in `-campfire` / `-torch` before `-frame...`), `-Intro <seconds>` the flight to the sea stack at the start of the level that many seconds into it instead (4.5: its end, the game's camera; the name ends in `-intro<seconds>`), `-Effect <bursts>` a burst of the events (names of `UghBursts.cpp` separated by commas, or `all`; one shot each, the name ending in `-<burst>`) held by the first copter - beside it in the air, on the ground or the water under it - and framed around it, `-EffectAge <seconds>` into it |
| `levels.ps1` | `-Quick` after a step: 12 levels showing every mood and feature (one player 1, 3, 6, 8, 12, 23, 36, 43, 62, the team's 1, 21, 54) on one sheet `Saved\Shots\Levels\levels-quick.png` (about 3 minutes); without it every level of both modes in one run (about half an hour: once a day at night or before a milestone), then the contact sheets `Saved\Shots\Levels\levels-1p.png` and `levels-team.png` (and the menu's `menu.png`): does every level look right? At the end the frame rates of the shots (median, slowest, fastest). `-Levels team:1-81` others |
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
| `Source/UghEditor/` | the editor's part: the commandlet `UghMakeAssets` that writes the materials of `UghMaterials.h` (clay, the drawing's rock, the cliff, fire, sprite card, the PBR master of the texture sets, a scanned model greyed, the sky (a cube); the water, the rain's streaks, its splashes and the logic's raindrops in `UghMakeWeather.cpp`; the springs' flowing water and a waterfall's mist in `UghMakeFlow.cpp`; the flame of a flipbook, its sparks, its smoke and burning wood in `UghMakeFire.cpp`; the blower's snort of dust and the flyer's wings the light shines through in `UghMakeFigures.cpp`; the bursts of the events - dust, smoke and spray, tumbling bits, fire, sparks and glints - in `UghMakeEffects.cpp`; `UghMaterialNodes` makes their nodes, the HLSL of the custom nodes is in `Shaders/`) the commandlet `UghMakeFlames` that bakes the flames (`FUghFireSim`: stable fluids on a staggered grid, fuel burning into heat that rises, swirls and cools, three slices one behind the other; `UghFlipbook`: cropped, looped, laid out, `UghFlames.h`), the commandlet `UghImportAssets` that imports the 3D assets (`UghAssetManifest` reads `Assets.json`; `UghNaniteMaterials` lets the models' materials draw Nanite meshes) the commandlet `UghCopyElectricDreams` that copies the assets of the Electric Dreams sample the game uses with their dependencies and the commandlet `UghExportElectricDreams` that exports some of them for the Blender scripts (their source meshes as OBJ, their materials' textures as PNG; `electric-dreams.ps1`) |

The frontend:

| Class | What it does |
|---|---|
| `AUghGameMode` | spawns the stage, the background, the figures, the campfire, the scenery and the speaker; the menu, then the logic every frame, its view and its sounds; builds the diorama of each level (behind the menu the one the menu would start, dimmed); at the first caption of a level the flight to the sea stack (`FUghIntro`, `AUghSeaStack`: the scene shown through the caption and the black before the play, a key hurries it; `-UghNoIntro` none); the keys of the frontend |
| `FUghMenu` | the menu before a game (the mode, the difficulty, a password): its keys and the game it would start (`FUghGameChoice`); the HUD draws it. Test `Ugh.Menu` (`UghMenuTests.cpp`) |
| `FUghPasswords` | the passwords of the levels of both modes (`assets/levels.json`) |
| `FUghSimulation` | the logic at its fixed tick: `Advance(seconds)` runs the steps that are due, keeps the views before and after the last step (`Alpha` between them) and the events of the steps |
| `FUghSounds` | the original's sounds (`assets/sound/*.wav`, read at run time) and which event of the logic plays which (`Cues`, as in the port; the other events had no sound) |
| `FUghMixer` | the sounds that play, mixed into one stream: the music (repeated, a delay, a fade-out) and four effects like the original's channels (a loop until its entity stops it); the volume. Unlike the original the effects do not take the music's voices |
| `FUghSoundPlayer` | when the game plays what: the menu's music, the level's music in the play, the effects of the events, the end of the play (the effects stop, the music fades out with the picture), a lost game's jingle, the ending's music. Tests `Ugh.Sounds.*` (`UghSoundTests.cpp`) |
| `AUghSpeaker` | the mixer's stream through the engine (a procedural sound wave fed a little ahead every frame); silent for the autopilot, with `-nosound` and without the files |
| `UghEvents` | each event of the logic's last steps to the sound player and the effects together (`Play`): an event plays its sound and shows its burst, from the same event |
| `FUghEffectPlayer` | which burst an event shows and where (`Cues`, the table below: Play, Loop until the event that stops it, Stop; at its entity's middle, feet, top or side it faces, under it on the water, at its player's copter - every copter for the level done -, the water line); an entity gone from the view is where it was last seen; what no event says, from the view: a copter coming down onto a pad faster than `LandingPixelsPerStep` raises dust, a bonus item landing a little, a crash into the water splashes too; the ground under a burst its bits come to rest on. Only decoration. Tests `Ugh.Effects.*` (`UghEffectTests.cpp`) |
| `UghBursts` | what each burst (`EUghBurst`) is made of: parts of particles of one look (`EShape`: a puff, a drop, a ring, a leaf, a feather, a shell, a chip, a glint, a petal, fire) and motion (thrown from a box along a cone, slowed by the air, falling or rising, rocking, tumbling), drawn translucent and lit (`UghMaterials::Burst`), cut out and lit (`Bits`) or as light (`Glint`); a flash of light, where a shot shows it |
| `AUghEffects` | shows the bursts: each part a mesh of tiny quads the material moves on the GPU (`UghBurst.hlsl`, `UghBurstLook.hlsl`; nothing a frame but its clock), a few of a kind at once (the oldest gives way), made when first wanted, hidden when over; a loop follows its entity; the flashes' lights (two, no shadows); the points earned rising (the HUD draws them); a burst held for a shot (`Hold`) |
| `FUghKeyboard` | a key event of the engine to the logic (Adapter): pilots' keys, Esc, P, any other key |
| `AUghPlayerController` | passes every key press and release to the game mode (the logic wants raw key events, not input actions) |
| `AUghStage` | the light and the air of a level's mood (`UghMood`): the sun or the moon, the sky (an HDR picture on a dome - the import makes a cube of it -, captured by the sky light; without it the engine's atmosphere), the fog with volumetric fog (shafts of sunlight into the cave), a mist over the water in a storm, a fixed exposure for each mood with a film look (gentle bloom, warm white balance, a little more saturation and contrast, a vignette); the plants of the Electric Dreams sample swaying harder and with the wind in a storm (its foliage's parameter collection); and the camera (`Fit`: fixed, a narrow lens, a little from above; a shot's close-up frames a part of the screen; on its flight to the stone with less motion blur and a darker exposure outdoors); the sky's dome far beyond the fog's reach (the sky shows over the open sea, as bright as `FUghMood::SkySeen`) |
| `FUghIntro` | the flight to the sea stack while a level's caption shows: its clock (it settles in black for a few frames, fades in, a key hurries it to its end in 0.6 s - before the play can begin) and the camera at a moment of it (`At`): low over the waves along a curve (Catmull-Rom through waypoints relative to the game's camera), at full speed, then braking to a stop, banking into its turns, bobbing a little, its lens narrowing from 72 to 30 degrees, looking along its way and at the level; from its end exactly the game's camera. Test `Ugh.Intro` (`UghIntroTests.cpp`) |
| `UghMood` | the mood of a level: a calm level's by its number in the mode's order (a day of `LevelsADay` levels: day, day, evening, evening, dusk, night), a windy level's a storm (a dim cool light, an overcast sky, a dense grey fog, a mist); the sun's angle, colour and lux, the sky (and how bright it is seen), the fog, the exposure, the caustics. Test `Ugh.Mood` |
| `AUghWater` | the water (`UghWater`): a sea of `UghMaterials::Water` (Single Layer Water) around the cliff, from near the camera to the cave's back wall and far beyond the screen's sides, its surface exactly at the logic's water level between two steps; its rain, wind, sunlight and caustics by the mood, the rings of what swims or floats on it, the foam and the waves where a waterfall pours in; while the camera flies in the open sea 4 km around (higher waves, a long swell and whitecaps far from the stone, mirroring the mood's sky itself). Test `Ugh.Water.Level` (`UghWeatherTests.cpp`) |
| `AUghRain` | the rain of a windy level (`UghRain`): thousands of streaks between the camera and the cliff that the material `UghMaterials::Rain` moves (a mesh of tiny quads; the GPU animates them), splashes on the ledges and on the water (`UghMaterials::Splash`), the logic's raindrops as streaks on cards (`UghMaterials::Raindrop`); all falling as far with the wind as down, as the logic's raindrops. Test `Ugh.Rain.Wind` |
| `UghStreams` | the springs of a level (`Plan`, `FUghStream`): where a ledge has rock under it all the way down under the water (its face drops into the sea, at least `MinDrop` pixels high; the stream's columns and `Margin` pixels around), room above it, the back wall behind it and the floor all the way to it, away from where copters land on pads, from the boards and the cave entrances - the highest such waterfall of the level, the same every time; what each keeps clear of decorations (`Rooms`), its channel (`Channel`: cut into the rock in front of and behind the slab of the play, `FUghRockField::CarveChannels`, and into the back wall as its hole), where its waterfall pours into the sea (`Feet`). Test `Ugh.Streams` (`UghStreamTests.cpp`) |
| `AUghFalls` | shows the springs: the stream flowing out of its hole along its channel across the ledge, through the rock of the slab of the play, out of a notch of the face's edge and down the face into the sea as a waterfall shooting out a little (one mesh of `UghMaterials::Flow`, its pattern moving with the water), the footbridge of logs over the channel (`UghFalls::Bridge`: their tops under the ledge's surface, a handrail behind the slab; `rough_wood`), the spray and mist at its foot (`UghMaterials::Mist`, a mesh of quads the material moves); the rising sea shortens the waterfall (the material: nothing under the surface) and swallows it |
| `UghMeshes` | static meshes built at run time (`Build`: vertices, triangles turned to their normals; `Quads`: upright quads with their own numbers) and a new component of an actor (`NewPart`) |
| `FUghRockMesh` | the rock of a level as a mesh: the surface of `FUghRockField` (`UghSurfaceNets`) in the world, its normals from the field, its UVs the screen (the drawing), its vertex colours how open (closed deep in a cave entrance's passage: dark) and how deep the surface is, how near below a top edge of the mask (the grass hanging over it) and its large patches. Test `Ugh.Rock` (`UghRockTests.cpp`) |
| `FUghRockField` | the rock of a level as a field on a grid (pixels, beyond the screen too; layers in depth): in the slab of the play the collision mask exactly, in front of it the rock's face with rounded edges in relief (fractured blocks of limestone, swellings: only there, in front of the slab), behind it the cave's walls and ceilings reaching further in the deeper they are, rough, the back wall far behind (deeper behind the drawing's dark holes), the arches and passages of its cave entrances, the channels of its streams (`CarveChannels`: in front of and behind the slab only), beyond the screen the cliff closing in |
| `FUghRockOutline` | the collision mask as signed distances on the grid of the pixels' centres (the edge exactly on the pixels' borders), and blurred, with the way they grow |
| `UghRockFeatures` | where a level's cave has stalactites (under flat ceilings with room below) and fallen rocks (at the feet of walls, not on pads), behind the slab of the play (none in front of a cave entrance) |
| `FUghCavePortal` | a cave entrance (`UghCavePortals::Plan`: one at each door of the drawing, `FUghLevelArt::Doors`, on the mask's floor under it): an arch of fractured rock standing out of the back wall 18 px behind the plane of the play (behind every rotor's and wing's sweep), widening into the wall, around an opening 22 px wide and high whose passage goes 72 px deep and turns aside, dark (no light gets far in); the passengers come out of it. Nothing may be seen in its passage (decorations but ground cover in front of its arch, the dressing). Test `Ugh.Portals` (`UghPortalTests.cpp`) |
| `UghRockNoise` | the noise of fractured limestone the rock is shaped with: blocks (cells of a jittered grid) with flat, tilted faces and cracks on some of their borders; smooth unions of shapes |
| `UghSurfaceNets` | the surface of the field as quads (surface nets): a point in every cell it passes, a quad across every edge of the grid it crosses; every point of the grid keeps its side (for the rock's grid and the sea stack's) |
| `UghLedges` | where things fit on the rock (its collision mask): dry ledges with room above, off the pads (or not) |
| `UghDecorations` | where a level's decorations stand (`Plan`) and the rules that keep them from hiding a figure: nothing on the screen nearer than `SlabFront` (25 units behind the plane of the play), nothing taller than ground cover (5 px) nearer than `FigureReach` (80: the copters' bodies 45, the enemies 70), nothing taller than 16 px and no liana nearer than `SweepReach` (180: a rotor 130, the flyer's wings 170), where a copter lands on a pad its body's and its rotor's room (`PadBody`, `PadRotor`); the same for the same level (its id seeds the choice). Test `Ugh.Scenery` (`UghSceneryTests.cpp`) |
| `UghPlans` | the parts of the plan, the bigger first: rocks and ferns at the springs' holes (`UghPlants.cpp`), campfires on the longest ledges without a pad, torches beside the cave entrances and on the back wall (`UghTorchPlan.cpp`), palms (the tallest that fit), totems and a hut (`UghDecorations.cpp`), bushes, ferns, jungle plants, rocks, stumps and bones along every ledge, meadows of grass with patches of flowers in rows from as near as they may be (`UghPlants.cpp`), lianas from the ceilings and curtains of them down the cave's back wall (`UghVines.cpp`) |
| `FUghPlacer` | whether a decoration fits: on the screen above the water, settled on the rock at its own depth (`FUghGround`), as deep as the rules want it there, not in another one's box (ground cover among ground cover may be; leafy plants mingle, keeping only their middles apart; grass grows at a palm's trunk), nowhere near a campfire |
| `FUghGround` | the rock of a level as ground at any depth (its field `FUghRockField`): the surface of a floor or a ceiling near a row of the mask, how deep the wall behind a point is, whether the front half of a box is air (its back may lean on the cave's back wall) |
| `AUghScenery` | shows the decorations: the scanned models of the Electric Dreams sample (`UghElectricDreams`) where it was copied, else the imported models (`UghAssets`), scaled into their boxes (a liana hanging from its top), one instanced component a mesh (thousands of tufts of grass: Nanite); the grass and the flowers cast no shadows, the cut-out leaves are not ray traced; clay shapes where a kind's models are not imported (a trunk with a crown, a stone, a bush, a pole, a cone) |
| `UghElectricDreams` | the scanned assets of the Electric Dreams sample the frontend uses, by their paths in the sample (`/Game/External/ElectricDreams/<path>` here): the decorations of each kind, the cliffs and roots of the dressing (the texture parameters of the cliffs' materials), the scanned grass, moss and soil textures of the cliff's layers, the taro whose leaves the people wear; the scans the Blender scripts make figures of (`ForBlender`); none (and a log line) without the copy |
| `UghRockDressing` | where the cave of a level is dressed (`Plan`): scanned cliffs (big, facing the camera, overlapping) on its back wall and roots hanging from its ceilings, each pushed into the wall with a part of it out (the cliffs on their sides: their columns lie as beds of limestone), smaller or none where it would be seen in a cave entrance's passage, none over a spring (`UghStreams`), never nearer than `BackFront` (250 units: behind every figure's sweep) nor than the middles of the decorations in front of it (but the ground cover and the lianas); the same for the same level. Test `Ugh.Dressing` (`UghDressingTests.cpp`) |
| `AUghCliffDressing` | shows the dressing: the scanned cliffs (greyed into the cliff's limestone with a little moss on what faces up: `UghMaterials::Scan` with their own textures) and roots scaled into their balls, turned, pushed into the wall, one instanced component a mesh (Nanite); nothing without the copy |
| `UghAssets` | the imported 3D assets the frontend asks for, by the id of `Assets.json` (`/Game/Imported/<id>`): their static meshes (or one by name), a skeletal mesh and its animations, a texture set's material instance, a sky's texture, the stone passenger (the scanned one, else the older); none (and a log line) when missing |
| `FUghLevelArt` | the original's drawing of a level: its tiles (`assets/levels.json`) composed from the sprites; the boards with the pads' numbers and the doors (a doorway, a cave's mouth: the cave entrances) among them |
| `UghPadSigns` | where the pads' boards stand (`Plan`): exactly one a pad, the original's board on the pad where it draws one (its bottom on the pad's surface; the first of two), else as near the pad's middle as it is out of the caves' entrances and the other boards; on its ground at `Depth` (88 units: out of the figures' reach), the pad's number as tally marks (`Marks`: one to four strokes, the four crossed by a fifth; a pad of number 6 the original's blank board). The decorations keep out of their way (`FUghPlacer`). Test `Ugh.Signs` (`UghSignTests.cpp`) |
| `AUghSigns` | the pads' boards: the models of `Blender/signs.py` (a weathered board of `rough_wood` pegged to a crooked post, the marks cut into it showing pale fresh wood), static, facing the camera; without them a card with the original's board |
| `AUghBackground` | the rock mesh in the cliff's material (each layer the scanned surface of `UghElectricDreams` where it has one - grass, moss, soil -, else its imported texture set - the limestone `marble_cliff_03`, the grey stone with lichen `mossy_rock` -, `UghMaterials::Cliff`, wet at the water's level; without them the drawing's colours, the pads' boards among them); the unseen shroud around the rock's grid that shades the cave (`ShroudBoxes`) |
| `AUghSeaStack` | the sea stack the level is carved into, seen only while the camera flies to it: the surface of `FUghStackField` in the cliff's material (its rock's surfaces larger; plain rock without it), wet at the water, its jungle (an `AUghScenery`); made the first time it shows (about a second), hidden in the play |
| `FUghStackField` | the sea stack as a field on a coarse grid (0.6 m): a tower of karst limestone about 108 m wide, 52 m deep and 90 m high in the open sea, rounded, lumpy, fluted, in beds with ledges and overhangs, fractured, a notch of the waves, its foot spreading under the sea; its face flat around the hollow the level's rock sits in (a frame in front of the rock's face, the rock's open border inside), standing out further away; its vertex colours (as the rock's); its jungle (`Plants`: palms, bushes, plants, ferns and rocks on its top, bushes on its ledges, ivy down its walls' tops). Test `Ugh.Stack` (`UghIntroTests.cpp`) |
| `AUghCampfire` | the campfires of the plan: a hearth (`FUghHearths`), a flame baked from a simulated fire playing on two crossed cards (`UghMaterials::Flame`, the flipbook `UghFlames::Campfire`; the card seen edge on fades out), sparks shooting up and a thin plume of smoke (`UghFireParts`), a light flickering in brightness, colour and place (Lumen tints the rock warm, the shadows dance; soft shadows), as bright as the mood wants (`FUghMood::FireLight`: the most at night and at dusk), each fire its own way; in the wind the flames lean and flicker more, the sparks and smoke drift; a fire goes out when the water rises over its ledge |
| `FUghHearths` | the campfires' hearths: a ring of scanned stones of the Electric Dreams sample around its branches burnt to charcoal leaning together over glowing coals (`UghMaterials::Embers`: black cracked charcoal, a little ash, embers glowing in the cracks and breathing, the logs' undersides more); without the sample Kenney's campfire in clay, without it two clay logs |
| `AUghTorches` | the torches of the plan: the torch of `Blender/torch.py` (its head of bark and pitch glowing, `UghMaterials::Embers`; clay without it) wedged into the rock, leaning out and a little aside, a small flame (the flipbook `UghFlames::Torch`), a few sparks, a wisp of smoke and a flickering light on the wall around it; out under the water |
| `UghFireParts` | what the campfires and torches share: the flicker of their lights (`FFlicker`: noise of a few speeds - brightness, colour temperature, place), the lights (soft point lights with shadows), the flame's crossed cards, the meshes of sparks and puffs of smoke (`UghMaterials::Sparks`, `Smoke`: quads the GPU moves, nothing a frame) |
| `AUghCopters` | the copters between two steps: the pedal copters of `Blender/copter.py` in the players' colours (orange, teal), the rotor and the pilot's crank turning (`FUghRotorSpin`), the pilot sitting turned to the left and pedalling (the action held at the crank's angle), the passenger sitting behind him turned to the right as a person of his look (the stone passenger smaller on the seat), or the stone passenger hanging in the sling, swaying against the copter's way; without the models clay boxes |
| `UghCopterModel` | where the model's parts are (the numbers of `Blender/copter_layout.py` in the world's units): the seats and how the people on them are turned, the crank's axle and the handles in the pilot's frame, the rotor's hub, the hanging stone, the stone on the seat |
| `FUghRotorSpin` | how far a rotor and its crank have turned: as fast as the rotor's sprites change lately (the logic only says which sprite), slowing to a stop without them |
| `FUghCaveman` | the people by their look (0 the pilots, 1 .. 3 the passengers by the logic's cargo look): the MetaHumans (`FUghMetaHuman`) where all of them are there, else the caveman of `Blender/caveman.py` (a `FUghRig`) dressed as the look (hair, beard, colours of hair, fur and skin by the material slots); actions idle, sit, pedal, hang, walk, wave, tread, swim, fall; a new person on an actor (a holder, the person `UghFigurePlace::PersonHeight` tall whatever the model: as big walking as a passenger's sprite, waiting, swimming and sitting in a copter), playing or holding an action |
| `FUghMetaHuman` | a MetaHuman of `metahumans.ps1` (`UghMetaHumans`: their names and folder): the components of its blueprint without the actor - the body playing its actions (a `FUghRig`), the face following its pose, the grooms on the face as hair cards (level of detail 3; no strands) without physics, none of them in the ray traced scene, its leaves (`FUghLeaves`) on the body's bones. Test `Ugh.Figures.People` (`UghPeopleTests.cpp`) |
| `FUghLeaves` | what the people wear instead of clothes: a loincloth of big leaves round the hips (the front and the sides on the thighs, going with the legs, lying on the lap of one sitting; the back and a short leaf in front of the crotch on the pelvis), for the woman a band of leaves round the chest; each leaf a sheet wrapped round the body a little away from it, flaring out, made at run time for each body from its rest pose and its physics asset (how thick it is), its picture a leaf of the Electric Dreams sample's taro (else green clay) |
| `FUghRig` | an animated model: a skeletal mesh and an animation per action (a model of the Blender scripts, `UghAssets`, or a MetaHuman's body); a component of it on an actor, playing an action or holding it at a part of its loop |
| `UghBetween` | render interpolation: a position between two steps (a jump is not interpolated), the view to interpolate from |
| `AUghFigures` | passengers, enemies, bonus items between two steps as the models of `FUghFigureModels` doing what `FUghFigureActions` say, plasticine shapes where a model is missing; speech bubbles as cards with their sharp pictures (`UghBubbles`); the clay riders of the clay copters |
| `UghBubbles` | a passenger's speech bubble: what it shows (`Look`: by the bubble's name in the data, `ugh_logic_get_sprite` - the board of the pad it wants to go to, a blank one from the sixth pad on, a question mark when the copter left without it), where it is (`Place`: as the original 11 px right of and 13 px above its passenger, its one tail on the left pointing down at the passenger's head - the original has no bubble with a tail on the right; mirrored to the passenger's left where it would leave the screen), its picture drawn from signed distances, 8 texels a pixel (`Draw`: a white bubble with a slate outline, the board's pale tally marks on brown wood, the question mark). Test `Ugh.Bubbles` (`UghSignTests.cpp`) |
| `FUghFigureActions` | which sprite means what (Table of rules over the names `ugh_logic_get_sprite` gives: "kind1.walkLeft" a caveman walking to the left, "kind1-water.walkRight" swimming, "flyer.left", "walker.chargeRight", "blower.blowing", "tree.swaying", "standingPassenger", "energy3" ...): the model, its action, which way it looks, whether the action follows the frames of the sprite's animation; from the entity an enemy knocked out (`stunned`) and a passenger going down in the water (falling). Test `Ugh.Figures.Actions` (`UghFigureTests.cpp`) |
| `FUghFigureModels` | the figures' models: the passengers as people of their look or the stone with eyes (the scanned rock, else the boulder: `UghAssets::Stone`), the pterodactyl, the triceratops, the blower, the tree (each a `FUghRig`: Jan's triceratops and T-rex and the scanned hornbeam where they were made, else the cartoon ones), the bonus items (a mesh each, named as their kind); a blower snorting (`FUghSnort`); the flyer's wings shining through (`UghMaterials::Membrane` with its maps); a component per entity, placed as its sprite (feet on its bottom, in the water at the surface, the flyer in its middle, banked and bigger; coming out of a door from behind), looking to its side and a little towards the camera, holding its action at the clock of its frames or playing it |
| `FUghSnort` | the dust and breath the T-rex snorts out of its nostrils while it blows out (after breathing in for the first 3 of its 10 sprites): a mesh of quads at its bone `nostrils` that `UghMaterials::Puff` moves out along their way from its head, fanning out, growing, fading |
| `FUghFrameClock` | where an action that follows a sprite's animation is: as fast as the frames change lately, never past the frame shown (like `FUghRotorSpin`) |
| `FUghSprites` | the sprites of the original: their sizes (`assets/sprites.json`) and pixels (`assets/sprites/NNN.png`) |
| `UghShapes` | where the screen of the original lies in the world (1 px = 10 units, X right, Z up, depth toward the camera negative), the slab of the play, shapes, cards with a sprite and materials |
| `UghTexture` | pixels as a texture |
| `UghMaterials` | the materials' paths and parameters, shared by the game and the commandlet |
| `FUghShot` | `-UghShot`: the game starts levels from the menu by itself (a player's keys), lets them hover, logs the frame rate and takes a screenshot of each (`shot.ps1`, `levels.ps1`, `pso.ps1`); a passenger shown in the copters, bubbles over the passengers and a close-up for a look at them; a look at the first campfire or torch (`-UghShotLook`); the flight to the sea stack at a moment (`-UghShotIntro`); a burst of the events held by the first copter (`-UghShotEffect`) |
| `FUghUpscaler` | DLSS / FSR / TSR at 67 % and the frame generation, as tried in the toolchain trial |
| `AUghHud` | the menu with how the last game ended; in a game the status line, the caption (with a shadow: over the flight to the sea stack), the keys; the upscaler and the volume |
| `UghJson` | reads a JSON file of the extracted data |

## The events seen

Every event of the logic plays its sound (`FUghSounds::Cues`, where the original had one) and shows its burst
(`FUghEffectPlayer::Cues`) from the same event (`UghEvents`); close-ups of each: `shot.ps1 -Effect all`.

| Event | Burst | Where |
|---|---|---|
| level caption | `surf`: waves breaking on the stone along the water line (seen from the flight) | the water line, before the face |
| copter crashed | `explosion`: fire, smoke, debris, sparks, a flash; into the water a `splash` too | the copter |
| level done | `celebration`: petals and glints, a flash | above every copter |
| passenger boarded | `dust`, small | the passenger's feet |
| passenger paid | `shells`: shell money thrown up, glinting; the points rising | the copter |
| quick delivery | `glints` | under the copter (the bonus item drops) |
| passenger dropped | `dust`, small | under the copter (the sling) |
| passenger in water | `splash`: drops, spray, rings | under the passenger on the water |
| flyer screech | `feathers`, fewer | the flyer |
| flyer flap start, stop | `downdraft`: dust and down under its wings, following it until it stops | the flyer |
| blower blow | `gust`: leaves, streaks of wind, dust along its breath | the side it faces |
| enemy stunned | `thud`: dust and pebbles (the flyer: `feathers`); the points rising | its feet |
| tree drop | `leaves` falling from the crown | the crown |
| bonus collected | `glints` tinted by its kind (energy, a life, the multiplier), a flash | where the item was |
| (a copter landing on a pad) | `dust`, the harder the bigger | under the copter |
| (a bonus item landing) | `dust`, small | under the item |

## 3D assets

Free assets (CC0, downloadable without an account: Poly Haven, ambientCG, Kenney, OpenGameArt) and a few local ones
(Jan's T-rex from Fab and triceratops from Sketchfab, put into `assets\3d` by hand; the scans `electric-dreams.ps1`
exports from Epic's sample), never in git. `Assets.json` lists each with its source, page, author, license, folder and
kind; `docs/visual-concept.md` says what they are for.

| Kind | Source | What the import makes of it (`Content\Imported\<id>`) |
|---|---|---|
| `model` | Poly Haven glTF (its API: the files of a resolution, checked by MD5) or an archive (size and SHA-256 in the manifest) | the files of `import` through Interchange: Nanite static meshes, their materials and textures; the materials are instances of copies of the engine's glTF (and FBX) materials in `Content\Imported\_Masters` that allow Nanite meshes and instances (the engine's do not: the game would draw them grey); a material that blends (glTF alphaMode BLEND: grass, flowers) is cut out instead (Nanite draws no translucency) |
| `texture` | Poly Haven maps or an ambientCG zip | the `maps` by role (color, normal, arm = occlusion/roughness/metal, roughness, ao, height) as textures with the right compression, and `MI_<id>`, an instance of `M_UghPbr` (`UghMaterials::Pbr`; the cliff takes its layers' textures from these) |
| `hdri` | Poly Haven `.hdr` | an HDR texture of the sky (long-lat) |
| `generated` | a Blender script of `Blender/` (`blender`: the script, its `inputs` - assets of the manifest it uses -, its `outputs`), no download | as a model: static meshes (Nanite), or a rigged mesh as a skeletal mesh with its skeleton and an animation per action; its materials allow skeletal meshes too |
| `local` | its `files` put there by hand (Jan's models) or by `electric-dreams.ps1` (the scans), never downloaded; without them it is absent and so is what is made of it | nothing: only an input of a generated asset |

Folders: `assets\3d\<source>\<asset>\` (the downloads; archives are kept in `assets\3d\_archives\` and extracted to
the asset's folder), `Blender\` (scripts in git that make what an asset lacks, run by `fetch-assets.ps1` with
`blender -b`: `palm.py` turns the OBJ palms of Nobiax's pack into glTF with a cut-out leaf material; `copter.py`
(with `copter_materials.py`, `copter_layout.py`), `caveman.py` (with `caveman_rig.py`, `caveman_actions.py`),
`stone_passenger.py`, `pterodactyl.py`, `triceratops.py`, `blower.py`, `fruit_tree.py` and `bonus_items.py` make the
copters, the caveman, the stone passenger, the enemies and the bonus items from nothing, `walker_triceratops.py`
(Jan's triceratops refined: its import fixed, subdivided, its skin baked, its animations as the walker's),
`blower_trex.py` (Jan's T-rex rigged and posed lying asleep, its eyes shut, its jaw closed), `tree_hornbeam.py` (a
trunk with a face carved into the scanned bark, the scanned crowns) and `stone_boulder.py` (the scanned rock with
eyes) the photoreal ones of the local assets, `bones.py`, `totem.py`,
`hut.py` (with `prop_shapes.py`: bone, skulls, painted wood) and `vines.py` the tribe's props and the lianas,
`signs.py` the pads' boards and `torch.py` the torches (the texture set `rough_wood`), sharing
`ugh_kit.py`
(procedural textures, materials, shapes, the glTF export), `ugh_blobs.py` (soft shapes of metaballs), `ugh_rig.py`
(armatures, skin weights - by distance or Blender's automatic ones -, actions keyed from poses with two-bone IK),
`ugh_bake.py` (colour, normal and roughness maps baked with Cycles from a procedural material) and `creature_kit.py`
(scaly hides, cartoon and wet eyes, lids, horns, stars over a dizzy head)),
`Content\Imported\<id>\` (the import, with `Import.stamp`: the files it was made from; the commandlet leaves an asset
alone while they stay the same, and deletes it before importing it again). On a clean machine:

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\fetch-assets.ps1
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\build.ps1
```

A new asset: an entry in `Assets.json` (and its id in `UghAssets.h` when the game asks for it), then both scripts.

### Epic's Electric Dreams sample

The photoreal look (step 18b) comes from Epic's free "Electric Dreams Environment" sample (Fab / Epic Games Launcher,
UE 5.8; Megascans and custom assets licensed for use in Unreal Engine projects - not ours to share, never in git). Its
project (about 56 GB) is only read: `electric-dreams.ps1` runs the commandlet `UghCopyElectricDreams`, which mounts the
sample's content read-only at `/UghSample/`, follows the hard dependencies of the assets of `UghElectricDreams.h`
through the asset registry and copies their package files (`.uasset`, `.uexp`, `.ubulk` ...) unchanged to
`Content\External\ElectricDreams\<path>` (about 260 packages, 3.1 GB; files no longer needed are deleted), then loads
each asset (the meshes' Nanite data is built once there; the textures are built by the first game that shows them). The sample's packages name each other by its `/Game/<path>`:
the `[CoreRedirects]` of `Config/DefaultEngine.ini` send `/Game/Megascans/`, `/Game/MSPresets/`, `/Game/Custom/`,
`/Game/SmartAssets/` and `/Game/PhysicalMaterials/` to the copy (the commandlet fails, naming the line, when a top
folder lacks its redirect). The sample's materials are Substrate materials, and its textures virtual ones: the project
renders with Substrate (`r.Substrate=True`; its own materials are converted) and without virtual textures
(`r.VirtualTextures=False`: the textures are built as plain ones, the materials sample them so, the cliff's custom
code can too). On a machine with the sample (re-creation of the copy):

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\build.ps1
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\electric-dreams.ps1
```

The script also exports the scans of `UghElectricDreams::ForBlender` from the copy (commandlet
`UghExportElectricDreams`: each mesh's source - the full scan, not Nanite's fallback - as an OBJ in Blender's axes, the
textures of its materials as PNG, their parameters as JSON) to `assets\3d\electricdreams` (the local asset
`electricdreams_scans`): `tree_hornbeam.py` and `stone_boulder.py` make the tree and the stone passenger of them when
`fetch-assets.ps1` runs next.

Without the copy the game shows the free assets above (and logs it once); everything builds and the tests pass.

### The MetaHumans

The pilot and the passengers (step 18d) are MetaHumans made with MetaHuman Creator (the engine's plugin
`MetaHumanCharacter`, enabled in `UghGame.uproject` with `PythonScriptPlugin`; its optional content "MetaHuman Creator
Core Data" from Fab / the Epic Games Launcher; licensed for Unreal Engine projects - not ours to share, never in git).
`metahumans.ps1` runs `Python/metahumans.py` in the editor without a window (a commandlet cannot bake the textures):
for each of `UghMetaHumans.h` (Pilot, Man, Woman, Grandpa) a copy of a preset (`Characters/<name>`), its hair and
beard, no outfit (an outfit cuts the body away under it; the game dresses them in leaves, `FUghLeaves`), the face rig
and the skin textures from Epic's cloud (the editor logged in to an Epic account), the assembly (pipeline Optimized,
quality Medium: hair cards) to `Content\External\MetaHumans\<name>` with the shared assets in `...\Common` (about
1.3 GB), the colours of the hair (its materials' parameters). Then `Python/metahuman_actions.py` (a commandlet)
makes each one's actions in its own proportions (`Actions/AS_<action>`, made for its body: the engine does not
retarget them): `idle` and `walk` from Creator's clips (the walk in place), the others posed by `metahuman_poses.py`
with `ugh_math.py` (two-bone IK, turns of the spine and the head, fingers; the copter's crank and handles of
`Blender/copter_layout.py`); where each action has its origin is as the caveman's. On a machine with Creator's content:

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\build.ps1
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\metahumans.ps1
```

Without them the game shows the caveman of `Blender/caveman.py` (and logs it once); everything builds and the tests
pass.

## Rules

- The plane of the play is the collision mask, exactly: in the slab of the play the rock's edge is on the borders of
  its solid pixels (test `Ugh.Rock`); nothing the frontend shows decides anything. The decorations (campfires too)
  stand behind the slab of the play and out of the figures' reach, so that they never hide a figure, a pad or its
  board (`UghDecorations`, test `Ugh.Scenery`); the scanned cliffs and roots dressing the cave stand on its back wall
  behind every figure's sweep and the decorations in front of them (`UghRockDressing`, test `Ugh.Dressing`). A copter's body stays within 45 units of the plane of the
  play, behind the rock's face (60 units in front of it) and the speech bubbles; its rotor sweeps a circle of 130
  units at the top of the body, in front of the face only above the air of the body (the face stands on solid pixels).
- The frontend reads the logic only through `ugh_logic.h`; values it needs (the screen, the copter's body, a full
  tank, the frame rate) come from there, checked against the logic by `static_assert` in `LogicApi.cpp`.
- The original's data stays out of git: the drawing, the sprites and the sounds are read from `assets\` at run
  time. The 3D assets stay out too (`assets\3d`, imported to `Content\Imported`; the Electric Dreams sample's copy and
  the MetaHumans in `Content\External`): the repository without them builds, passes its tests and shows the free
  assets and the caveman, or clay shapes, the drawing's colours on the rock and the engine's sky instead.
- FSR is an upscaler only (`r.FidelityFX.FI.Enabled=0`, `OverrideSwapChainDX12=0` in `Config/DefaultEngine.ini`),
  so it does not clash with DLSS frame generation.
