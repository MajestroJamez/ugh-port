# UGH! in Unreal Engine 5.8 - the prehistoric diorama

The game logic of `../logic` inside Unreal Engine, shown as a diorama (`docs/visual-concept.md`): the level is a
cliff cut exactly in the plane of the play (its edge there is the collision mask), a smooth rock with rounded edges,
its cave reaching back to a far back wall dressed with scanned sandstone cliffs and hanging roots, stalactites and
fallen rocks, in scanned rock, soil, grass and moss steered by the original's drawing (its face layered sandstone in
relief, grass and moss hanging over its edges, wet at the water), the cliff and the water going on beyond the screen
under an HDR sky, a stone age jungle on its ledges (meadows of grass and flowers, palms with big leaves, bushes, ferns, jungle plants, rocks, stumps, bones, totems, a hut, campfires with flickering flames) and ivy
from its ceilings and down its back wall, all behind the figures, the boards with the pads' numbers as the original
draws them; the scanned assets come from Epic's Electric Dreams sample where it was copied (`electric-dreams.ps1`),
else free ones; the copters are stone age pedal
copters of bamboo, wicker, leather, bone and stone (made by Blender scripts) with a stone age man pedalling them and
their passengers sitting behind him or hanging below; the passengers are stone age people (a man, a woman, an old man)
walking, waving, waiting, swimming and falling as their sprites say (the standing passenger a stone with eyes) -
photoreal MetaHumans made with MetaHuman Creator where they were made (`metahumans.ps1`), else a cartoon caveman; the
enemies a pterodactyl, a triceratops, a
puffing beast and a tree with a face, the bonus items fruits and a stone tablet (all made by Blender scripts, clay
shapes the size of their sprites without them), a passenger's speech bubble a card with its sprite. The logic runs at
the original's tick (70.086 Hz) whatever the frame rate; every frame is drawn
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
| `fetch-assets.ps1` | downloads the 3D assets of `Assets.json` to `assets\3d` (see 3D assets below): only what is missing, each file checked by its size and hash, the whole under the budget of 100 GB, archives extracted, Blender scripts run (a generated asset is made by its script, again when a script of `Blender/` is newer than it); a table of the assets at the end. `-Only <ids>` some, `-Verify` hashes the files already there too |
| `electric-dreams.ps1` | copies the scanned assets of Epic's Electric Dreams sample the game uses (`UghElectricDreams.h`: cliffs, roots, palms, jungle plants, ferns, ivy, grass, stones, rock surfaces; about 3 GB) with everything they need into `Content\External\ElectricDreams` (commandlet `UghCopyElectricDreams`, see 3D assets below); `-Source <folder>` the sample's project (default `..\Unreal Projects\ElectricDreamsEnv` next to the repository). After `build.ps1`; again after a change of `UghElectricDreams.h` |
| `metahumans.ps1` | makes the pilot and the passengers with MetaHuman Creator in the editor (without a window) into `Content\External\MetaHumans`: `Python\metahumans.py` (presets of Creator's optional content dressed as stone age people, their face rig and skin textures from Epic's cloud - the editor logged in to an Epic account -, assembled; only the missing ones, `-All` all again) and `Python\metahuman_actions.py` (an animation per action for each); see The MetaHumans below. After `build.ps1` |
| `build.ps1` | builds the editor (modules UghLogic, UghGame, UghEditor), makes the materials (commandlet `UghMakeAssets` -> `Content\Generated`) and imports the downloaded 3D assets (commandlet `UghImportAssets` -> `Content\Imported`); `-NoAssets` only builds, `-ForceImport` imports every asset again |
| `test.ps1` | the automation tests inside the engine without a window and without sound (`UnrealEditor-Cmd -nullrhi -nosound`): the golden replays `Ugh.Replays.*` (they need `.\gradlew.bat :verify:replays`), the menu `Ugh.Menu`, the sounds `Ugh.Sounds.*` (every event's file, the mixer, when what plays) the decorations `Ugh.Scenery` (every level rich - at least 150 of 6 kinds, 100 tufts of grass and flowers, a palm -, the same every time, each on the rock at its own depth with its box in the air, none on the screen nearer than the slab of the play, where the figures reach, where a rotor or a flyer's wing sweeps, where a copter lands on a pad or in front of a pad's board; nothing in a campfire), the rock `Ugh.Rock` (in every level the rock's mesh cut in the slab of the play is the collision mask: every pixel's centre on its side, every other point at most half a pixel off - the cut leaves the corners of the pixel steps), the rock's dressing `Ugh.Dressing` (every level has scanned cliffs, the same every time, each coming out of the cave's back wall, behind every figure's sweep and behind the middles of the decorations in front of it), the copters `Ugh.Copter.*` (the rotor turns as fast as its sprites change; the imported model fills the copter's body and stays in it, the stone passenger is as big as its sprite) and the figures `Ugh.Figures.*` (every sprite an entity of the data shows is an action of a model, `FUghFigureActions`; the actions that follow the sprites' frames go on smoothly; the enemies' models are as big as their sprites, the bonus items are there), the people `Ugh.Figures.People` (the MetaHumans' actions put them where the game wants them: on the ground and in the slab of the play, on the seat, the feet on the copter's pedals and the hands on its handles, the hands on the rope, the head at the water's surface); `-Filter` other tests |
| `shot.ps1` | starts a level from the menu by itself without a window, lets the copters hover (the log says the frame rate meanwhile) and saves `Saved\Shots\<mode>-<NN>.png`: `-Level <n>` (from 1), `-Team`, `-At <seconds>` later, `-Commands "<cvar> <value>"` to try a setting, `-Cargo <look>` the copters shown with a passenger of that look in the cabin (1 .. 3; `-Hanging` below instead: 4, the stone; only the picture, the autopilot never picks one up), `-CloseUp` framing the copters, `-Frame <left>,<top>,<width>,<height>` that part of the screen (pixels: a look at the figures; the name then ends in `-cargo<look>` or `-hanging<look>`, `-closeup`, `-frame<left>_<top>`) |
| `levels.ps1` | the same for every level of both modes in one run, then the contact sheets `Saved\Shots\Levels\levels-1p.png` and `levels-team.png` (and the menu's `menu.png`): does every level look right? At the end the frame rates of the shots (median, slowest, fastest). `-Levels team:1-81` fewer |
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
| `Source/UghEditor/` | the editor's part: the commandlet `UghMakeAssets` that writes the materials of `UghMaterials.h` (clay, the drawing's rock, the cliff, water, fire, sprite card, the PBR master of the texture sets, the sky; `UghMaterialNodes` makes their nodes, the HLSL of the custom nodes is in `Shaders/`) the commandlet `UghImportAssets` that imports the 3D assets (`UghAssetManifest` reads `Assets.json`; `UghNaniteMaterials` lets the models' materials draw Nanite meshes) and the commandlet `UghCopyElectricDreams` that copies the assets of the Electric Dreams sample the game uses with their dependencies (`electric-dreams.ps1`) |

The frontend:

| Class | What it does |
|---|---|
| `AUghGameMode` | spawns the stage, the background, the figures, the campfire, the scenery and the speaker; the menu, then the logic every frame, its view and its sounds; builds the diorama of each level (behind the menu the one the menu would start, dimmed); the keys of the frontend |
| `FUghMenu` | the menu before a game (the mode, the difficulty, a password): its keys and the game it would start (`FUghGameChoice`); the HUD draws it. Test `Ugh.Menu` (`UghMenuTests.cpp`) |
| `FUghPasswords` | the passwords of the levels of both modes (`assets/levels.json`) |
| `FUghSimulation` | the logic at its fixed tick: `Advance(seconds)` runs the steps that are due, keeps the views before and after the last step (`Alpha` between them) and the events of the steps |
| `FUghSounds` | the original's sounds (`assets/sound/*.wav`, read at run time) and which event of the logic plays which (`Cues`, as in the port; the other events had no sound) |
| `FUghMixer` | the sounds that play, mixed into one stream: the music (repeated, a delay, a fade-out) and four effects like the original's channels (a loop until its entity stops it); the volume. Unlike the original the effects do not take the music's voices |
| `FUghSoundPlayer` | when the game plays what: the menu's music, the level's music in the play, the effects of the events, the end of the play (the effects stop, the music fades out with the picture), a lost game's jingle, the ending's music. Tests `Ugh.Sounds.*` (`UghSoundTests.cpp`) |
| `AUghSpeaker` | the mixer's stream through the engine (a procedural sound wave fed a little ahead every frame); silent for the autopilot, with `-nosound` and without the files |
| `FUghKeyboard` | a key event of the engine to the logic (Adapter): pilots' keys, Esc, P, any other key |
| `AUghPlayerController` | passes every key press and release to the game mode (the logic wants raw key events, not input actions) |
| `AUghStage` | the sun, the sky (an HDR picture on a dome, captured by the sky light; without it the engine's atmosphere), the fog, a fixed exposure with a film look (gentle bloom, warm white balance, a little more saturation and contrast, a vignette) and the camera (fixed, a narrow lens, a little from above; a shot's close-up frames a part of the screen); a windy level is a storm (a dim cool sun, a dark cloudy sky, a dense grey fog) |
| `FUghRockMesh` | the rock of a level as a mesh: the surface of `FUghRockField` (`UghSurfaceNets`) in the world, its normals from the field, its UVs the screen (the drawing), its vertex colours how open and how deep the surface is, how near below a top edge of the mask (the grass hanging over it) and its large patches. Test `Ugh.Rock` (`UghRockTests.cpp`) |
| `FUghRockField` | the rock of a level as a field on a grid (pixels, beyond the screen too; layers in depth): in the slab of the play the collision mask exactly, in front of it the rock's face with rounded edges in relief (layers of sandstone, joints, swellings: only there, in front of the slab), behind it the cave's walls and ceilings reaching further in the deeper they are, rough, the back wall far behind (deeper behind the drawing's dark holes), beyond the screen the cliff closing in |
| `FUghRockOutline` | the collision mask as signed distances on the grid of the pixels' centres (the edge exactly on the pixels' borders), and blurred, with the way they grow |
| `UghRockFeatures` | where a level's cave has stalactites (under flat ceilings with room below) and fallen rocks (at the feet of walls, not on pads), behind the slab of the play |
| `UghSurfaceNets` | the surface of the field as quads (surface nets): a point in every cell it passes, a quad across every edge of the grid it crosses; every point of the grid keeps its side |
| `UghLedges` | where things fit on the rock (its collision mask): dry ledges with room above, off the pads (or not) |
| `UghDecorations` | where a level's decorations stand (`Plan`) and the rules that keep them from hiding a figure: nothing on the screen nearer than `SlabFront` (25 units behind the plane of the play), nothing taller than ground cover (5 px) nearer than `FigureReach` (80: the copters' bodies 45, the enemies 70), nothing taller than 16 px and no liana nearer than `SweepReach` (180: a rotor 130, the flyer's wings 170), where a copter lands on a pad its body's and its rotor's room (`PadBody`, `PadRotor`); the same for the same level (its id seeds the choice). Test `Ugh.Scenery` (`UghSceneryTests.cpp`) |
| `UghPlans` | the parts of the plan, the bigger first: campfires on the longest ledges without a pad, palms (the tallest that fit), totems and a hut (`UghDecorations.cpp`), bushes, ferns, jungle plants, rocks, stumps and bones along every ledge, meadows of grass with patches of flowers in rows from as near as they may be (`UghPlants.cpp`), lianas from the ceilings and curtains of them down the cave's back wall (`UghVines.cpp`) |
| `FUghPlacer` | whether a decoration fits: on the screen above the water, settled on the rock at its own depth (`FUghGround`), as deep as the rules want it there, not in another one's box (ground cover among ground cover may be; leafy plants mingle, keeping only their middles apart; grass grows at a palm's trunk), nowhere near a campfire |
| `FUghGround` | the rock of a level as ground at any depth (its field `FUghRockField`): the surface of a floor or a ceiling near a row of the mask, how deep the wall behind a point is, whether the front half of a box is air (its back may lean on the cave's back wall) |
| `AUghScenery` | shows the decorations: the scanned models of the Electric Dreams sample (`UghElectricDreams`) where it was copied, else the imported models (`UghAssets`), scaled into their boxes (a liana hanging from its top), one instanced component a mesh (thousands of tufts of grass: Nanite); the grass and the flowers cast no shadows, the cut-out leaves are not ray traced; clay shapes where a kind's models are not imported (a trunk with a crown, a stone, a bush, a pole, a cone) |
| `UghElectricDreams` | the scanned assets of the Electric Dreams sample the frontend uses, by their paths in the sample (`/Game/External/ElectricDreams/<path>` here): the decorations of each kind, the cliffs and roots of the dressing, the scanned sandstone, rock, grass, moss and soil textures of the cliff's layers; none (and a log line) without the copy |
| `UghRockDressing` | where the cave of a level is dressed (`Plan`): scanned cliffs (big, facing the camera, overlapping) on its back wall and roots hanging from its ceilings, each pushed into the wall with a part of it out, never nearer than `BackFront` (250 units: behind every figure's sweep) nor than the middles of the decorations in front of it (but the ground cover and the lianas); the same for the same level. Test `Ugh.Dressing` (`UghDressingTests.cpp`) |
| `AUghCliffDressing` | shows the dressing: the scanned cliffs and roots scaled into their balls, turned, pushed into the wall, one instanced component a mesh (Nanite); nothing without the copy |
| `UghAssets` | the imported 3D assets the frontend asks for, by the id of `Assets.json` (`/Game/Imported/<id>`): their static meshes (or one by name), a skeletal mesh and its animations, a texture set's material instance, a sky's texture; none (and a log line) when missing |
| `FUghLevelArt` | the original's drawing of a level: its tiles (`assets/levels.json`) composed from the sprites; the boards with the pads' numbers among them |
| `AUghBackground` | the rock mesh in the cliff's material (the layers of the scanned surfaces of `UghElectricDreams`, else of the imported texture sets, `UghMaterials::Cliff`, wet at the water's level; without them the drawing's colours), the boards with the pads' numbers as sprite cards, the water |
| `AUghCampfire` | the campfires of the plan: Kenney's stones and logs in darker clay (two clay logs without them), a flame of three crossed cards in the material `UghMaterials::Fire` (tongues licking upwards through rising noise, sparks; `Shaders/UghFlame.hlsl`), a flickering point light each (Lumen), each fire its own way; in the wind the flames lean and flicker more; a fire goes out when the water rises over its ledge |
| `AUghCopters` | the copters between two steps: the pedal copters of `Blender/copter.py` in the players' colours (orange, teal), the rotor and the pilot's crank turning (`FUghRotorSpin`), the pilot pedalling (the action held at the crank's angle), the passenger sitting behind him as a person of his look, or the stone passenger hanging in the sling, swaying against the copter's way; without the models clay boxes |
| `UghCopterModel` | where the model's parts are (the numbers of `Blender/copter_layout.py` in the world's units): the seats, the crank's axle, the rotor's hub, the hanging stone |
| `FUghRotorSpin` | how far a rotor and its crank have turned: as fast as the rotor's sprites change lately (the logic only says which sprite), slowing to a stop without them |
| `FUghCaveman` | the people by their look (0 the pilots, 1 .. 3 the passengers by the logic's cargo look): the MetaHumans (`FUghMetaHuman`) where all of them are there, else the caveman of `Blender/caveman.py` (a `FUghRig`) dressed as the look (hair, beard, colours of hair, fur and skin by the material slots); actions idle, sit, pedal, hang, walk, wave, tread, swim, fall; a new person on an actor (a holder as high as the caveman whatever the model), playing or holding an action |
| `FUghMetaHuman` | a MetaHuman of `metahumans.ps1` (`UghMetaHumans`: their names and folder): the components of its blueprint without the actor - the body playing its actions (a `FUghRig`), the face and the outfit following its pose, the grooms on the face as helmets (level of detail 5) without physics; the outfit in the caveman's leopard fur. Test `Ugh.Figures.People` (`UghPeopleTests.cpp`) |
| `FUghRig` | an animated model: a skeletal mesh and an animation per action (a model of the Blender scripts, `UghAssets`, or a MetaHuman's body); a component of it on an actor, playing an action or holding it at a part of its loop |
| `UghBetween` | render interpolation: a position between two steps (a jump is not interpolated), the view to interpolate from |
| `AUghFigures` | passengers, enemies, bonus items between two steps as the models of `FUghFigureModels` doing what `FUghFigureActions` say, plasticine shapes where a model is missing; raindrops (strokes of clay along their way with the wind); speech bubbles as sprite cards; the clay riders of the clay copters |
| `FUghFigureActions` | which sprite means what (Table of rules over the names `ugh_logic_get_sprite` gives: "kind1.walkLeft" a caveman walking to the left, "kind1-water.walkRight" swimming, "flyer.left", "walker.chargeRight", "blower.blowing", "tree.swaying", "standingPassenger", "energy3" ...): the model, its action, which way it looks, whether the action follows the frames of the sprite's animation; from the entity an enemy knocked out (`stunned`) and a passenger going down in the water (falling). Test `Ugh.Figures.Actions` (`UghFigureTests.cpp`) |
| `FUghFigureModels` | the figures' models: the passengers as cavemen of their look or the stone with eyes, the pterodactyl, the triceratops, the blower, the tree (each a `FUghRig`), the bonus items (a mesh each, named as their kind); a component per entity, placed as its sprite (feet on its bottom, in the water at the surface, the flyer in its middle, banked and bigger; coming out of a door from behind), looking to its side and a little towards the camera, holding its action at the clock of its frames or playing it |
| `FUghFrameClock` | where an action that follows a sprite's animation is: as fast as the frames change lately, never past the frame shown (like `FUghRotorSpin`) |
| `FUghSprites` | the sprites of the original: their sizes (`assets/sprites.json`) and pixels (`assets/sprites/NNN.png`) |
| `UghShapes` | where the screen of the original lies in the world (1 px = 10 units, X right, Z up, depth toward the camera negative), the slab of the play, shapes, cards with a sprite and materials |
| `UghTexture` | pixels as a texture |
| `UghMaterials` | the materials' paths and parameters, shared by the game and the commandlet |
| `FUghShot` | `-UghShot`: the game starts levels from the menu by itself (a player's keys), lets them hover, logs the frame rate and takes a screenshot of each (`shot.ps1`, `levels.ps1`, `pso.ps1`); a passenger shown in the copters and a close-up for a look at them |
| `FUghUpscaler` | DLSS / FSR / TSR at 67 % and the frame generation, as tried in the toolchain trial |
| `AUghHud` | the menu with how the last game ended; in a game the status line, the caption, the keys; the upscaler and the volume |
| `UghJson` | reads a JSON file of the extracted data |

## 3D assets

Free assets only (CC0, downloadable without an account: Poly Haven, ambientCG, Kenney, OpenGameArt), never in git.
`Assets.json` lists each with its source, page, author, license, folder and kind; `docs/visual-concept.md` says what
they are for.

| Kind | Source | What the import makes of it (`Content\Imported\<id>`) |
|---|---|---|
| `model` | Poly Haven glTF (its API: the files of a resolution, checked by MD5) or an archive (size and SHA-256 in the manifest) | the files of `import` through Interchange: Nanite static meshes, their materials and textures; the materials are instances of copies of the engine's glTF (and FBX) materials in `Content\Imported\_Masters` that allow Nanite meshes and instances (the engine's do not: the game would draw them grey); a material that blends (glTF alphaMode BLEND: grass, flowers) is cut out instead (Nanite draws no translucency) |
| `texture` | Poly Haven maps or an ambientCG zip | the `maps` by role (color, normal, arm = occlusion/roughness/metal, roughness, ao, height) as textures with the right compression, and `MI_<id>`, an instance of `M_UghPbr` (`UghMaterials::Pbr`; the cliff takes its layers' textures from these) |
| `hdri` | Poly Haven `.hdr` | an HDR texture of the sky (long-lat) |
| `generated` | a Blender script of `Blender/` (`blender`: the script, its `inputs` - texture sets of the manifest it uses -, its `outputs`), no download | as a model: static meshes (Nanite), or a rigged mesh as a skeletal mesh with its skeleton and an animation per action; its materials allow skeletal meshes too |

Folders: `assets\3d\<source>\<asset>\` (the downloads; archives are kept in `assets\3d\_archives\` and extracted to
the asset's folder), `Blender\` (scripts in git that make what an asset lacks, run by `fetch-assets.ps1` with
`blender -b`: `palm.py` turns the OBJ palms of Nobiax's pack into glTF with a cut-out leaf material; `copter.py`
(with `copter_materials.py`, `copter_layout.py`), `caveman.py` (with `caveman_rig.py`, `caveman_actions.py`),
`stone_passenger.py`, `pterodactyl.py`, `triceratops.py`, `blower.py`, `fruit_tree.py` and `bonus_items.py` make the
copters, the caveman, the stone passenger, the enemies and the bonus items from nothing, `bones.py`, `totem.py`,
`hut.py` (with `prop_shapes.py`: bone, skulls, painted wood) and `vines.py` the tribe's props and the lianas, sharing
`ugh_kit.py`
(procedural textures, materials, shapes, the glTF export), `ugh_blobs.py` (soft shapes of metaballs), `ugh_rig.py`
(armatures, skin weights, actions keyed from poses with two-bone IK) and `creature_kit.py` (scaly hides, eyes, horns,
stars over a dizzy head)),
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

Without the copy the game shows the free assets above (and logs it once); everything builds and the tests pass.

### The MetaHumans

The pilot and the passengers (step 18d) are MetaHumans made with MetaHuman Creator (the engine's plugin
`MetaHumanCharacter`, enabled in `UghGame.uproject` with `PythonScriptPlugin`; its optional content "MetaHuman Creator
Core Data" from Fab / the Epic Games Launcher; licensed for Unreal Engine projects - not ours to share, never in git).
`metahumans.ps1` runs `Python/metahumans.py` in the editor without a window (a commandlet cannot bake the textures):
for each of `UghMetaHumans.h` (Pilot, Man, Woman, Grandpa) a copy of a preset (`Characters/<name>`), its hair and
beard, the face rig and the skin textures from Epic's cloud (the editor logged in to an Epic account), the assembly
(pipeline Optimized, quality Low) to `Content\External\MetaHumans\<name>` with the shared assets in `...\Common`
(about 1.5 GB), the colours of the hair (its materials' parameters). Then `Python/metahuman_actions.py` (a commandlet)
makes each one's actions in its own proportions (`Actions/AS_<action>`): `idle` and `walk` from Creator's clips (the
walk in place), the others posed by `metahuman_poses.py` with `ugh_math.py` (two-bone IK, turns of the spine and the
head, fingers; the copter's crank and handles of `Blender/copter_layout.py`); where each action has its origin is as
the caveman's. On a machine with Creator's content:

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
