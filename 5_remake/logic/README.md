# UGH! game logic (C++)

The game logic of UGH! (1992), written anew in C++20 without dependencies. It computes every frame exactly as the
original DOS game does - same keys in, same state out, frame by frame - but it knows nothing of the original's
memory: it reads named game data and keeps its state in classes of the game's concepts. No drawing and no sound: a
frontend (Unreal Engine, step 10 of `docs/plan.md`) reads the state through the C API `include/ugh_logic.h` and
plays sounds and effects from the events.

Exactness is checked by the golden replays: 161 recordings of the original (every level of both modes, 434 000
frames) with the semantic state after every frame, which the logic must reproduce field by field.

## Build and test

Windows PowerShell 5.1, with VS Build Tools (MSVC, CMake, Ninja):

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\logic\build.ps1
```

It builds `build\ugh_logic.lib` and the tests `build\ugh_logic_tests.exe`, then runs them (by module, `tests/`).
`-NoTest` only builds. The tests need the game data `assets\logic\ugh-data.ugd` (`.\gradlew.bat :extractor:run`).
The golden replays are checked by `6_verification\build.ps1` (`6_verification/README.md`).

## Modules

A folder is a module and a namespace (`src/passengers/route/Waiting.hpp` is `ugh::passengers::route::Waiting`); one
class per file, named like the file; includes start at `src/`. A module uses only the modules above it:

| Module | What is in it |
|---|---|
| `units/` | the arithmetic of the original: `Fixed` (a position in 1/32 px) and `Speed` (1/64 Fixed per frame) wrap at 16 bits like the original (`Int16` inside them); `Countdown`. Everything else is a plain `int` |
| `state/` | `State` and `StateMachine`: the state of an entity and how its states change it (templates, used by every entity) |
| `data/` | the game data, read-only: `DataFileReader` reads and checks `ugh-data.ugd`, `GameData` holds the levels (`LevelDefinition` with its pads and the placements of passengers and enemies), the kinds (passengers: `RoutePassengerKind`, `SwimmerKind`, `StandingPassengerKind`), animations, keys, rules. Every record is a struct with public fields, read-only through `GameData`; the placements also have `accept` (Visitor) |
| `events/` | `Event`s for the frontend (sounds, effects) and their listeners; `Diagnostics` for what the logic does not support |
| `world/` | the world of the game: `Session` (lives, score, level number, random numbers), `Level` (copters, pads, water, rain, energy, fade - and the questions the entities ask about it), `Copter`, `Pad`, `Water`, `Rain`, `Figure` (what a passenger or an enemy shows: position, sprite, animation), `Animator`, `PlayContext` |
| `physics/` | `CopterPhysics` (one frame of a copter's flight), `CollisionProbe` (a copter against the background), `TouchBox` (a copter against a sprite), `Ballistics` (anything thrown that falls) |
| `bonuses/` | the bonus items: `BonusSlots`, `BonusItem`, their states `Falling` and `Lying` |
| `passengers/` | `Passengers`, the base `Passenger`; `route/` the passenger with a route (17 states, also in the water; its parts `Route`, `PassengerCall`, `Ride`, `Swim`), `standing/` the standing passenger (5 states) |
| `enemies/` | `Enemies`, the base `Enemy`, `EnemyFactory`; `flyer/`, `walker/`, `blower/`, `tree/`: each kind its class and states |
| `input/` | `MenuKey`, `MenuInput`: the keys the game loop looks at (Esc gives up, P would pause, a caption waits for any key); the pilots' keys go straight to the copters (`Game::key`) |
| `game/` | `Game` (the facade), `GameFlow` with its `phases/`, `PlayFrame` (one frame of the play), `Cheats` (the test pilot of the replays) |
| `api/` | `LogicApi.cpp`: the C API over `Game` |

`tests/` holds the tests by module. The fields of the replays (`ugh_logic_replay`) and the replay check live in
`6_verification/`; the logic does not know them.

## One frame

`Game::step()` is one frame of the original (70.086 Hz). `GameFlow` knows where the game is:

```
new game -> BlackBeforeCaption (8 frames) -> CaptionFadeIn (the attempt starts: the level is loaded; 65 frames)
-> CaptionWaitKey (until a key) -> CaptionFadeOut (65) -> BlackBeforePlay (8) -> Playing (until the fade-out is over)
-> the attempt is over: CaptionFadeIn of the next attempt, or the end of the game
```

Before the first frame of the play the enemies and then the passengers get one update, and nothing is shown. Each
frame of the play (`PlayFrame::run`) runs the systems in the order of the original:

1. the fade, the water
2. the last menu key (Esc gives the game up)
3. the copters (`CopterPhysics::fly`) - not while the level is still fading in
4. the passengers, the enemies, the bonus items (each runs its state's `update()`)
5. where the passengers were seen, the rotors, the rain, the water surface

## Patterns

- **State**: a passenger, an enemy or a bonus item has a state object (`passengers::route::Waiting` ...) with
  `enter()` (when the entity gets into it) and `update()` (every frame). The states are stateless singletons; the
  entity holds the data. The entity derives from `state::StateMachine<Entity, Context>` (one template for all
  of them): `changeState(next, context)` runs the entry action now and the update from the next frame on;
  `continueIn(next, context)` runs both now. The states derive from `state::State<Entity, Context>`. The game flow is a state machine of `Phase`s too.
- **Visitor**: the placements of a level (`data::PassengerPlacementVisitor`, `EnemyPlacementVisitor`) and the
  entities by type (`passengers::PassengerVisitor`, `enemies::EnemyVisitor`) - no RTTI.
- **Factory**: `PassengerFactory`, `EnemyFactory` make the right class from a placement.
- **Parameter Object**: `world::PlayContext` (and `PassengerContext`, `EnemyContext`) is what an update gets.
- **Observer**: the logic reports `events::Event`s to `EventListener`s.
- **Facade**: `game::Game` is the one entry; nothing of the state can be set from outside but through `Cheats`.

No exceptions, no RTTI, no macros (the library builds as an Unreal Engine module), templates only where they remove
copies (`state/`): errors come back as values (`DataFileReader::read` returns nullptr and the text of the error).

## Where to change what

| I want to ... | Go to |
|---|---|
| change the gravity, the lift, the steering of a copter, how hard it can land | `src/physics/CopterPhysics.cpp` (the constants at the top); the crash limits are in the data (`rules`) |
| make a passenger call a copter for longer | `src/passengers/route/Calling.hpp` (`CALL_TIME`) |
| change how many lives a game starts with | `src/world/Session.hpp` (`START_LIVES`) |
| change what a state of a passenger or an enemy does | the state's file: `src/passengers/route/<State>.cpp`, `src/enemies/<kind>/<State>.cpp` |
| add an event for the frontend | `src/events/EventKind.hpp` (the kind), `context.report({...})` where it happens, `include/ugh_logic.h` (`UGH_LOGIC_EVENT_...`, in the same order) |
| add a field to the replays | the entity's writer in `6_verification/replay/` (`PassengerFields.cpp` ...: its rule and its value) and the same field in `4_test_data/verify/.../replay/SemanticProjection.kt` |
| add a kind of enemy | a folder `src/enemies/<kind>/` (the class and its states, like `flyer/`), its placement in `src/data/` (with `EnemyPlacementVisitor` and `DataFileReader`), `src/enemies/EnemyFactory.cpp`; then its fields in `6_verification/replay/EnemyFields.cpp` |
| change the order of the systems in a frame | `src/game/PlayFrame.cpp` |
| change the caption, the black screens, the end of an attempt | `src/game/phases/`, `src/game/GameFlow.cpp` (`endAttempt`) |
| change how a copter hits walls | `src/physics/CollisionProbe.cpp` |
| change what a frontend gets to draw | `include/ugh_logic.h` (`ugh_logic_view`) and `src/api/LogicApi.cpp` |

## Glossary

- **Original**: the DOS game. Which C++ class does what the original's routine did, and its Kotlin port: the map
  `docs/logic-map.md`.
- **Fixed**: a position in 1/32 px; **Speed**: 1/64 of a Fixed per frame (copters, swimmers).
- **Attempt**: one try at a level, from its caption until its fade-out reaches black (the level done, a crash, Esc).
- **Seen position** (`seenX`, `seenY`): where a passenger with a route was shown at the end of the last frame; its
  states decide from it, and a hidden passenger keeps it.
- **Quirk**: a behaviour of the original that looks like a bug but is kept, named where it happens (the collision
  probe looking only one pixel ahead going left or up, raindrops blown into the next row of the page ...). The list:
  `docs/rewrite-design.md`, chap. 6.
- **Golden replay** (`UGR 1`): keys per frame and the semantic state after it; **T line** a frame, **I line** what
  the test pilot set (a copter, the energy, the lives). **UGD 1**: the game data file.
