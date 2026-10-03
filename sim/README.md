# UGH! game logic core (C++)

The game logic of UGH! (1992) as a C++20 library without dependencies: copter physics, passengers, enemies, bonus
items, water, rain, keyboard and the flow of a game, frame by frame, exactly as the original DOS game computes them.
No drawing and no sound: a frontend (Unreal Engine from step 10 of `re/notes/plan.md`) reads the state and the events
through the C API in `include/ugh_sim.h`.

Exactness is checked by the golden replays: recordings of the original's state, frame by frame, that the core must
reproduce field by field (`verify/src/test/kotlin/ugh/verify/replay/`).

## Build and test

Windows PowerShell 5.1, with VS Build Tools (MSVC, CMake, Ninja):

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\sim\build.ps1
```

It builds `build\ugh_sim.lib`, the replay player `build\ugh_replay.exe` and the unit tests `build\ugh_sim_tests.exe`,
then runs CTest: the unit tests, and two tests per golden replay (the whole game; every transition on its own).
`-NoTest` only builds. It needs the game data `assets\sim\ugh-sim.bin` (`.\gradlew.bat :extractor:run`) and the
replays in `verify\build\replays` (`.\gradlew.bat :verify:replays`).

## Modules

A folder is a module and a namespace (`src/model/Copter.hpp` is `ugh::model::Copter`); one class per file, named
like the file. Includes start at `src/`.

| Module | What is in it |
|---|---|
| `core/` | building blocks: `Word` (a 16-bit word that wraps like a register), `Fixed` (a position in 1/32 px), `Speed` (1/64 Fixed per frame), `Countdown`, `Random`, `Event` and its listeners, `Diagnostics` |
| `data/` | the game data, read-only: `GameDataLoader` reads `ugh-sim.bin` and checks it, `GameData` holds the levels and the kinds of passengers, enemies and bonus items |
| `model/` | the state of a game: `GameSession` (lives, score, level number), `Level` (the running attempt and the questions the entities ask about each other), `Copter`, `Pad`, `Passenger`, `Enemy`, `BonusItem`, `Water`, `Rain`, `Fade` |
| `physics/` | `CopterPhysics` (one frame of a copter's flight), `CollisionProbe` (the copter against the background), `TouchBox` (a copter against a sprite) |
| `passengers/` | the passengers' state machine: one class per state (`walking/`, `swimming/`, `standing/`) |
| `enemies/` | the enemies: `EnemyBehavior` per type (`flyer/`, `walker/`, `blower/`, `tree/`) and their states |
| `bonuses/` | the bonus items' states: `Falling`, `Lying` |
| `input/` | `Keyboard`: scancodes to the keys the players hold |
| `game/` | `Game` (the facade for the C API), `GameFlow` with its phases (`phases/`), `PlayFrame`, `LevelLoader` |
| `replay/` | the only place that knows the golden replays: `ReplayProjection` and one field list per entity (`CopterFields` ...) |
| `api/` | `SimApi.cpp`: the C API over `Game` and `ReplayProjection` |

`tools/ugh_replay/` is the replay player, `tests/` the unit tests (one folder per module).

## Patterns

- **State**: a passenger, an enemy or a bonus item has a state object (`passengers::Waiting` ...) with `enter()`, run
  once when the entity gets the state, and `update()`, run every frame. The states are stateless singletons
  (`Waiting::instance`); the entity holds the data. The game flow is a state machine too (`game::FlowPhase`).
- **Strategy**: `enemies::EnemyBehavior` places an enemy of its type and gives its first state.
- **Memento**: every entity has a `Snapshot` (all its data) with `snapshot()` and `restore()`. The replay adapter reads
  and writes entities only this way.
- **Visitor**: `replay::FieldVisitor` runs over the field list of an entity; `FieldWriter` writes the fields as text,
  `FieldReader` sets one, `FieldFiller` fills them with a pattern.
- **Observer**: the logic reports `core::Event`s (sounds, crashes, deliveries) to `core::EventListener`s.
- **Facade**: `game::Game` is the one entry to the logic; the C API reaches the logic only through it.

## One frame

`Game::step()` is one frame of the original (70.086 Hz): the work from one wait for the vertical retrace to the next.
`GameFlow` knows where the game is:

```
new game -> BlackScreen (8 frames) -> CaptionFadeIn (the level loaded; 65 frames) -> CaptionWaitKey (until a key)
-> CaptionFadeOut (65) -> BlackScreen (8) -> Playing (a frame each until the fade-out is over)
-> the attempt over (GameFlow::endAttempt): CaptionFadeIn of the next attempt, or the end of the game
```

While `Playing`, a frame (`PlayFrame::run`) runs the systems in the order of the original:

1. the fade (in or out) and the water level
2. the keys (Esc gives up)
3. the copters (`CopterPhysics::fly`), unless the level is still fading in
4. the passengers, the enemies, the bonus items (each runs its state's `update()`)
5. the passengers' pixel positions, the rotors, the rain, the water surface

## Where to change what

| I want to ... | Go to |
|---|---|
| change the gravity, the lift, the steering, the crash | `src/physics/CopterPhysics.cpp` (the constants at the top) |
| change how a copter hits walls | `src/physics/CollisionProbe.cpp` |
| make a passenger call a copter for longer | `src/passengers/walking/Calling.cpp` (`CALL_TIME`) |
| change what a state of a passenger / an enemy does | the state's file: `src/passengers/<group>/<State>.cpp`, `src/enemies/<type>/<State>.cpp` |
| add an event for the frontend | `include/ugh_sim.h` (the `UGH_SIM_EVENT_...` value) and `src/core/Event.hpp`, then `level.report(...)` where it happens |
| add a field to the replays | the entity's list in `src/replay/<Entity>Fields.cpp` (one line), and the same field in the Kotlin `StateProjection.kt` |
| add an enemy type | `data/EnemyKind.hpp` (`Type`), `data/GameDataLoader.cpp` (recognize its descriptor), a folder `enemies/<type>/` with its `EnemyBehavior` and states, `EnemyBehavior::of`, its name in `replay/EnemyFields.cpp` |
| change the order of the systems in a frame | `src/game/PlayFrame.cpp` |
| change the caption, the black screens, the level end | `src/game/phases/`, `src/game/GameFlow.cpp` (`endAttempt`) |

## Glossary

- **Original**: the DOS game; `113b:xxxx` in a comment is the address of its routine (code segment 113b), `2a2d` and
  similar four-digit numbers are addresses of its data (DGROUP). The Kotlin port in `core/` names the same routines
  (`Passengers.kt ...`) and is the reference the replays were checked with.
- **Word**: a 16-bit value of the original; it wraps on overflow. **Fixed**: a position in 1/32 px.
- **Retrace**: the vertical retrace of the VGA monitor; the original waits for it once per frame.
- **Descriptor**: the record of a kind in the original's data (a passenger kind, an enemy kind ...); its offset
  (`origin`) is its name in the replays.
- **Replay**: a recording of the original's state (format "UGR 0"); **T line** the state after a frame, **B line**
  what a stage changed, **I line** values set from outside.
- **Quirk**: a behaviour of the original that looks like a bug but is kept, named where it happens (e.g. the
  collision probe moving left or up looks only one pixel ahead).
