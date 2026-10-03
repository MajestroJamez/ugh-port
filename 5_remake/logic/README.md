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

It builds `build\ugh_logic.lib`, the test pilot `build\ugh_logic_testing.lib` and the tests
`build\ugh_logic_tests.exe`, then runs them (by module, `tests/`).
`-NoTest` only builds. The tests need the game data `assets\logic\ugh-data.ugd` (`.\gradlew.bat :extractor:run`).
The golden replays are checked by `6_verification\build.ps1` (`6_verification/README.md`).

## Modules

A folder is a module and a namespace (`src/passengers/route/Waiting.hpp` is `ugh::passengers::route::Waiting`); one
class per file, named like the file; includes start at `src/`. A module uses only the modules above it (like Java
packages that never import "down"):

| Module | What is in it |
|---|---|
| `units/` | the arithmetic of the original: `Fixed` (a position in 1/32 px) and `Speed` (1/64 Fixed per frame) wrap at 16 bits like the original (`Int16` inside them); `Countdown`. Everything else is a plain `int` |
| `state/` | `State` and `StateMachine`: the state of an entity and how its states change it (templates, used by every entity) |
| `data/kinds/` | the kinds of the game data: of passengers (`RoutePassengerKind`, its `SwimmerKind` in the water, `StandingPassengerKind`), of enemies (`FlyerKind`, `WalkerKind`, `BlowerKind`, `TreeKind`) and of bonus items (`BonusKind`), and what they are made of (`Animation`, `AnimationPair`, `Box`). Every record of the data is a struct with public fields, read-only through `GameData` |
| `data/levels/` | a level as the data defines it: `LevelDefinition` with its `PadDefinition`s, `CollisionMask`, `Wind`, and the placements of passengers (`Route`) and enemies; the placements have `accept` (Visitor) |
| `data/` | the game data, read-only: `GameData` (the levels in the order of both modes, the kinds, `Rules`, `SpriteIds`), `Difficulty` |
| `data/ugd/` | reading `ugh-data.ugd` (format UGD 1) with all checks: `DataFileReader` puts together `UgdTokenizer` (lines to records), `RecordReader` (values and errors with the line) and the readers, each with a table of the record types it reads: `KindsReader` (with `AnimationsReader`, `PassengerKindsReader`), `RulesReader`, `LevelReader` (with `PlacementReader`). The keys of the PC keyboard are skipped (`6_verification/keyboard`) |
| `events/` | `Event`s for the frontend (sounds, effects) and their listeners; `Diagnostics` for what the logic does not support |
| `input/` | `PlayerKey` (the keys a pilot flies with), `MenuKey`, `MenuInput`: the keys the game loop looks at (Esc gives up, P would pause, a caption waits for any key) |
| `world/` | the world of the game: `Session` (lives, `Score`, level number, random numbers), `Level` (`Copters` - and what the entities ask about them, as `Copter*` -, `Pad`s, water, rain, energy, fade, `Delivery`), `Copter` (its player, `Motion`, `Controls`, `Rotor`, `Cabin`), `Pad` (its index, who waits on it), `Water`, `Rain`, `Figure` (what a passenger or an enemy shows: position, sprite, animation), `Animator`, `PlayContext` |
| `physics/` | `CopterPhysics` (one frame of a copter's flight), `CollisionProbe` (a copter against the background), `TouchBox` (a copter against a sprite), `Ballistics` (anything thrown that falls) |
| `bonuses/` | the bonus items: `BonusSlots`, `BonusItem`, their states `Falling` and `Lying` |
| `passengers/` | `Passengers`, the base `Passenger`; `route/` the passenger with a route (17 states, also in the water; its parts `RouteKinds`, `Route`, `PassengerCall`, `Ride`, `Swim`; `OnPickupPad` the base of the states on the pickup pad), `standing/` the standing passenger (5 states) |
| `enemies/` | `Enemies`, the base `Enemy`, `EnemyFactory`, `Stun` (a walker or a blower stunned); `flyer/`, `walker/`, `blower/`, `tree/`: each kind its class and states |
| `game/` | `Game` (the facade), `GameState` (what changes during a game), `GameFlow` with its `phases/` and `Attempts` (what the phases do to the game state: start an attempt, play a frame, end it), `PlayFrame` (one frame of the play) |
| `api/` | `LogicApi.cpp`: the C API over `Game` |

`testing/` is not part of the logic: `testing::TestPilot` (library `ugh_logic_testing`), the test pilot of the replays
(it puts a copter anywhere, keeps the energy and the lives up) for the tests and `6_verification`; the classes it
changes only name it as a `friend`. `tests/` holds the tests by module. The fields of the replays (`ugh_logic_replay`) and the replay check live in
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
  `continueIn(next, context)` runs both now. The states derive from `state::State<Entity, Context>`. The game
  flow is a state machine of `Phase`s too.
- **Visitor**: the placements of a level (`data::PassengerPlacementVisitor`, `EnemyPlacementVisitor`) and the
  entities by type (`passengers::PassengerVisitor`, `enemies::EnemyVisitor`) - no RTTI.
- **Factory**: `PassengerFactory`, `EnemyFactory` make the right class from a placement.
- **Parameter Object**: `world::PlayContext` is what an update gets; `PassengerContext` and `EnemyContext` derive from
  it and add what only they need (the bonus items, the passengers), so a state writes `context.level`.
- **Observer**: the logic reports `events::Event`s to `EventListener`s.
- **Facade**: `game::Game` is the one entry; nothing of the state can be set from outside but by the test pilot
  (`testing::TestPilot`, a test peer outside `src/`).
- **Table of methods**: each data reader has a table "record type -> its method" (`RecordTable`, like method
  references in Java): a new record type is one line in the table and one method.
- **Template Method**: `passengers::route::OnPickupPad`: the states on the pickup pad check the water and the copters
  every frame, then do their own part (`walk`, `stay`).

No exceptions, no RTTI, no macros (the library builds as an Unreal Engine module), templates only where they remove
copies (`state/`, `RecordTable`): errors come back as values (`DataFileReader::read` returns nullptr and the
text of the error).

## Rules of the code

- One class per file, named like the file; no file over 200 lines, no class over about 15 public methods (the
  facade `Game` aside): a class that grows gets parts with their own behaviour (`Copter` has `Rotor` and `Cabin`,
  `RoutePassenger` has `Route`, `PassengerCall`, `Ride`, `Swim`), and the states call their intents
  (`passenger.ride().start(...)`), not setters.
- No `protected` data: what the kinds of an entity share is a base class with its own methods (`world::Figure`).
- Objects, not indexes: the logic hands copters and pads around (`Copter&`, `Copter*`, `Pad&`); a number only where
  the world outside needs one (events, the C API, the replays: `Copter::player()`, `Pad::index()`) or the data names
  one (`passenger.route().pickupPad(level)`).
- No `friend` into the insides of another class: the factories of their own collections and the test pilot only.
- Nothing for the tests in `src/`: the test pilot lives in `testing/`.
- Values of the game are plain `int`s; only `Fixed` and `Speed` keep the 16 bits of the original (and the energy, as
  a named quirk). A quirk of the original lives in the class it belongs to, named and described.
- The logic knows nothing of DOS, the PC keyboard or the replays: those are in `6_verification/`.

## Where to change what

| I want to ... | Go to |
|---|---|
| change the gravity, the lift, the steering of a copter, how hard it can land | `src/physics/CopterPhysics.cpp` (the constants at the top); the crash limits are in the data (`rules`) |
| make a passenger call a copter for longer | `src/passengers/route/Calling.hpp` (`CALL_TIME`) |
| make a passenger wave impatiently for longer (no copter with room) | `src/passengers/route/Impatient.hpp` (`WAVE_TIME`) |
| change how many lives a game starts with | `src/world/Session.hpp` (`START_LIVES`) |
| change what a state of a passenger or an enemy does | the state's file: `src/passengers/route/<State>.cpp`, `src/enemies/<kind>/<State>.cpp` |
| add an event for the frontend | `src/events/EventKind.hpp` (the kind), `context.report({...})` where it happens, `include/ugh_logic.h` (`UGH_LOGIC_EVENT_...`, in the same order) |
| add a field to the replays | the entity's writer in `6_verification/replay/` (`PassengerFields.cpp` ...: its rule and its value) and the same field in `4_test_data/verify/.../replay/SemanticProjection.kt` |
| add a kind of enemy | a folder `src/enemies/<kind>/` (the class and its states, like `flyer/`), its kind in `src/data/kinds/` and placement in `src/data/levels/` (with `EnemyPlacementVisitor`), one line and one method in the tables of `src/data/ugd/KindsReader.cpp` and `PlacementReader.cpp`, `src/enemies/EnemyFactory.cpp`; then its fields in `6_verification/replay/EnemyFields.cpp` |
| read a new kind of record of the data | one line in the table of the reader it belongs to (`src/data/ugd/KindsReader.cpp`, `PlacementReader.cpp` ...) and its method |
| change how long a walker or a blower stays stunned | `src/enemies/Stun.hpp` (`TIME`) |
| change how long a bonus item lies on a pad | `src/bonuses/Lying.cpp` (`LYING_TIME`) |
| change the order of the systems in a frame | `src/game/PlayFrame.cpp` |
| change the caption, the black screens, the end of an attempt | `src/game/phases/`, `src/game/Attempts.cpp` (`end`) |
| change how a copter hits walls | `src/physics/CollisionProbe.cpp` |
| change what a frontend gets to draw | `include/ugh_logic.h` (`ugh_logic_view`) and `src/api/LogicApi.cpp` |
| change how the score multiplier works | `src/world/Score.hpp` |
| change what a passenger does on its pickup pad every frame (the water, a copter flying into it) | `src/passengers/route/OnPickupPad.cpp` |

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
