# UGH! game logic (C++)

The game logic of UGH! (1992), written anew in C++20 without dependencies. It computes every frame exactly as the
original DOS game does - same keys in, same state out, frame by frame - but it knows nothing of the original's
memory: it reads named game data and keeps its state in classes of the game's concepts. No drawing and no sound: a
frontend (Unreal Engine, `5_remake/game`, which compiles these sources as its module `UghLogic`) reads the state and
the background of the level (its pads and solid pixels) through the C API `include/ugh_logic.h` and
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
class per file, named like the file; includes start at `src/`. A module uses only the modules above it in the table
(like Java packages that never import "down"). A parent module and its sub-modules:

- `data/` and `world/` come after their sub-modules and use them (`GameData` holds the kinds and the levels, `Level`
  the copters and the scenery); `data/ugd/` comes after `data/`.
- `passengers/` and `enemies/` are in two parts. The base part comes before the sub-modules, and the sub-modules use
  only it: the base class, the context and the visitor (`Passenger`, `PassengerContext`, `PassengerVisitor`; `Enemy`,
  `EnemyContext`, `EnemyVisitor`, and `Stun`, which the walker and the blower share). The collection and the factory
  come after the sub-modules and use them (`Passengers`, `PassengerFactory`; `Enemies`, `EnemyFactory`).

| Module | What is in it |
|---|---|
| `units/` | the arithmetic of the original: `Fixed` (a position in 1/32 px) and `Speed` (1/64 Fixed per frame) wrap at 16 bits like the original (`Int16` inside them); `Countdown`. Everything else is a plain `int` (and the score, a `uint32_t`) |
| `state/` | `State` and `StateMachine`: the state of an entity and how its states change it (templates, used by every entity) |
| `data/kinds/` | the kinds of the game data: of passengers (`RoutePassengerKind`, its `SwimmerKind` in the water, `StandingPassengerKind`), of enemies (`FlyerKind`, `WalkerKind`, `BlowerKind`, `TreeKind`) and of bonus items (`BonusKind`), and what they are made of (`Animation`, `AnimationPair` with the `Facing` it shows, `Box`). Every record of the data is a struct with public fields, read-only through `GameData` |
| `data/levels/` | a level as the data defines it: `LevelDefinition` with its `PadDefinition`s, `CollisionMask`, `Wind`, `ScreenSize` (a level is one screen, 320 x 192 px: the one place of that size), and the placements of passengers (with their `Route`) and enemies; the placements have `accept` (Visitor) |
| `data/` | the game data, read-only: `GameData` (the levels in the order of both modes, the kinds, `Rules`, `SpriteIds`), `Difficulty` |
| `data/ugd/` | reading `ugh-data.ugd` (format UGD 1) with all checks: `DataFileReader` puts together `UgdTokenizer` (lines to records), `RecordReader` (values and errors with the line) and the readers: `KindsReader` (with `AnimationsReader`, `PassengerKindsReader`), `RulesReader`, `LevelReader` (with `PlacementReader`); the kinds of the enemies, the rules and the sprites must be there once. The keys of the PC keyboard are skipped (`6_verification/keyboard`) |
| `events/` | `Event`s for the frontend (sounds, effects) and their listeners; `Diagnostics` for what the logic does not support |
| `input/` | `PlayerKey` (the keys a pilot flies with), `MenuKey`, `MenuInput`: the keys the game loop looks at (Esc gives up, P would pause, a caption waits for any key) |
| `world/session/` | what lasts from one level to the next: `Session` (players, difficulty, level number) with `Lives`, `Score`, `RandomNumbers` |
| `world/figure/` | what an entity shows: `Figure` (the base of a passenger, an enemy and a bonus item: its index, position, sprite, animation), `Animator` |
| `world/scenery/` | the level around the entities: `Pad` (its index, who waits on it), `Water`, `Rain` with its `Raindrop`s, `Screen` (the edges past which a thing is gone) |
| `world/copter/` | `Copter` (its player, and its parts `Motion`, `Controls`, `Rotor`, `Cabin` with its `Cargo`; the `Pad` it stands on), `CopterShape` (its door, skids, body, outline, waterline - the one place of the copter's geometry), `Copters` (and what the entities ask about them, as `Copter*`) |
| `world/` | `Level` (the world of the level being played: `Copters`, `Pad`s, water, rain, `Energy`, `Fade`, `Delivery`; how an attempt ends: `passengerFinished`, `crash`, `fadeOut`), `PlayContext` |
| `physics/` | `CopterPhysics` (one frame of a copter's flight), `CollisionProbe` (a copter against the background), `TouchBox` (a copter against a sprite), `Ballistics` (anything thrown that falls) |
| `bonuses/` | the bonus items: `BonusSlots`, `BonusItem`, their states `Falling` and `Lying` |
| `passengers/` | `Passengers`, the base `Passenger`; `route/` the passenger with a route (17 states, also in the water; its parts `PassengerForm` (its kind on land or in the water), `RouteProgress`, `PickupWait` (waiting for a copter and calling it), `Ride`, `Swim`; `OnPickupPad` the base of the states on the pickup pad), `standing/` the standing passenger (5 states) |
| `enemies/` | `Enemies`, the base `Enemy`, `EnemyFactory`, `Stun` (a walker or a blower stunned); `flyer/`, `walker/`, `blower/`, `tree/`: each kind its class and states |
| `game/` | `Game` (the facade), `GameState` (what changes during a game), `GameFlow` with its `phases/` and `Attempts` (what the phases do to the game state: start an attempt, play a frame, end it), `PlayFrame` (one frame of the play) |
| `api/` | `LogicApi.cpp`: the C API over `Game`; `LevelView` what it shows of the level being played (`ugh_logic_view`: the entities with a passenger's look and an enemy knocked out, through Visitors) and what each sprite of the entities and each speech bubble is in the data (`ugh_logic_get_sprite`) |

`testing/` is not part of the logic: `testing::TestPilot` (library `ugh_logic_testing`), the test pilot of the replays
(it puts a copter anywhere, keeps the energy and the lives up) for the tests and `6_verification`; it is the one
`friend` of the facade `Game` and otherwise uses the public operations of the world. `tests/` holds the tests by module. The fields of the replays (`ugh_logic_replay`) and the replay check live in
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
- **Visitor**: the placements of a level (`data::levels::PassengerPlacementVisitor`, `EnemyPlacementVisitor`) and the
  entities by type (`passengers::PassengerVisitor`, `enemies::EnemyVisitor`) - no RTTI.
- **Factory**: `PassengerFactory`, `EnemyFactory` make the right class from a placement.
- **Parameter Object**: `world::PlayContext` is what an update gets; `PassengerContext` and `EnemyContext` derive from
  it and add what only they need (the bonus items, the passengers), so a state writes `context.level`.
- **Observer**: the logic reports `events::Event`s to `EventListener`s.
- **Facade**: `game::Game` is the one entry; nothing of the state can be set from outside but by the test pilot
  (`testing::TestPilot`, a test peer outside `src/` and the one friend of `Game`).
- **Table of methods**: the readers of records that stand alone (`KindsReader`, `RulesReader`, `PlacementReader`) have
  a table "record type -> its method" (`RecordTable`, like method references in Java): a new record type is one line
  in the table and one method. `LevelReader` reads a level and the parts after it in order, `AnimationsReader` one
  type: they pick by type in their code.
- **Template Method**: `passengers::route::OnPickupPad`: every frame the states on the pickup pad check the water (it
  may fall in), do their own `walk`, check a copter in the air (it may knock it in), then do their own `stay`.

No exceptions, no RTTI, no macros (the library builds as an Unreal Engine module; the one macro, `UGH_LOGIC_API` in
the C API, exports it from that module), templates only where they remove
copies (`state/`, `RecordTable`): errors come back as values (`DataFileReader::read` returns nullptr and the
text of the error).

## Rules of the code

- One class per file, named like the file; no file over 200 lines, no class over about 15 public methods (the
  facade `Game` aside): a class that grows gets parts with their own behaviour (`Copter` has `Rotor` and `Cabin`,
  `RoutePassenger` has `PassengerForm`, `RouteProgress`, `PickupWait`, `Ride`, `Swim`), and the states call their
  intents (`passenger.ride().start(...)`, `passenger.startCalling(...)`), not setters. Two states that start the same
  way (on a pad and in the water) call the same intent, not each other's `enter`.
- No `protected` data: what the kinds of an entity share is a base class with its own methods (`world::figure::Figure`).
- Objects, not indexes: the logic hands copters and pads around (`Copter&`, `Copter*`, `Pad&`); a number only where
  the world outside needs one (events, the C API, the replays: `Copter::player()`, `Pad::index()`) or the data names
  one (the placements). The route and the walker hold the pads of the level (`passenger.route().pickupPad()`,
  `walker.pad()`), a pad holds who waits on it (`const Figure*`).
- No `friend` into the insides of another class: the factories of their own collections, and the test pilot as the
  one friend of the facade `Game`.
- One place per fact: the copter's geometry is `world::copter::CopterShape`, the size of the screen
  `data::levels::ScreenSize`, how an attempt ends `world::Level` (`passengerFinished`, `crash`, `fadeOut`); values that
  only happen to be equal keep their own names there.
- No flag parameters: a `bool` that picks what a method does is two methods (`bounceFallingPassenger`,
  `bounceFallingPassengerUnseen`) or a small enum (`Enemy::Rebound`), so a call reads without its declaration.
- One unit, one type: a position or a speed per frame is a `Fixed`, a copter's or a swimmer's speed a `Speed`; frames
  are counted down by a `Countdown` (`tick`, or `tickToZero` for a delay that stays due). The one exception is the
  delay of the water surface (`world::scenery::Water`): it runs `SURFACE_DELAY` .. 0 and acts below 0, as the field
  the replays show. A method that counts a frame down says so: `tick...` (`stun().tick()`, `flyer.tickWait()`,
  `route().tickArrival()`), once per frame, true when the time ran out.
- A duration lives in the .cpp of the state that starts it (`Lying.cpp` `LYING_TIME`, `Screeching.cpp`
  `SCREECH_TIME`), or in the part that counts it when several states start it (`PickupWait::CALL_TIME`, `Stun::TIME`).
- The same name in two modules only for the states the replays name so (`flyer::Falling`, `bonuses::Falling`,
  `standing::Falling` ...): the namespace tells them apart, like a Java package.
- Nothing for the tests in `src/`: the test pilot lives in `testing/`. It sets state through the constructors that
  take a whole value - `world::copter::Motion` with its pixel position, `world::Energy(int)` -, the only ones the logic
  itself does not call.
- Values of the game are plain `int`s - but the score, a `uint32_t` (it grows past 16 bits and never goes below
  0); only `Fixed` and `Speed` keep the 16 bits of the original (and the energy, as a named quirk). A quirk of the
  original lives in the class it belongs to, named and described.
- The logic knows nothing of DOS, the PC keyboard or the replays: those are in `6_verification/`.

## Where to change what

| I want to ... | Go to |
|---|---|
| change the gravity, the lift, the steering of a copter, how hard it can land | `src/physics/CopterPhysics.cpp` (the constants at the top); the crash limits are in the data (`rules`) |
| make a passenger call a copter for longer | `src/passengers/route/PickupWait.hpp` (`CALL_TIME`) |
| make a passenger wave impatiently for longer (no copter with room) | `src/passengers/route/PickupWait.hpp` (`WAVE_TIME`) |
| change how many lives a game starts with | `src/world/session/Lives.hpp` (`START`) |
| change what a state of a passenger or an enemy does | the state's file: `src/passengers/route/<State>.cpp`, `src/enemies/<kind>/<State>.cpp` |
| add an event for the frontend | `src/events/EventKind.hpp` (the kind), `context.report({...})` where it happens, `include/ugh_logic.h` (`UGH_LOGIC_EVENT_...`, in the same order) |
| add a field to the replays | the entity's writer in `6_verification/replay/` (`PassengerFields.cpp` ...: its rule and its value) and the same field in `4_test_data/verify/.../replay/SemanticProjection.kt` |
| add a kind of enemy | a folder `src/enemies/<kind>/` (the class and its states, like `flyer/`) and its `visit` in `src/enemies/EnemyVisitor.hpp`; its kind in `src/data/kinds/` with a field and an accessor in `src/data/GameData.hpp` (`Contents`), and its placement in `src/data/levels/` (with `EnemyPlacementVisitor`); one line and one method in the tables of `src/data/ugd/KindsReader.cpp` (and its type in `ENEMY_KINDS` there) and `PlacementReader.cpp`; `src/enemies/EnemyFactory.cpp`. A falling standing passenger hits it in the one box of all enemies (`src/passengers/standing/StandingPassenger.cpp`); then its fields in `6_verification/replay/EnemyFields.cpp` |
| read a new kind of record of the data | one line in the table of the reader it belongs to (`src/data/ugd/KindsReader.cpp`, `PlacementReader.cpp` ...) and its method |
| change how long a walker or a blower stays stunned | `src/enemies/Stun.hpp` (`TIME`) |
| change how long a bonus item lies on a pad | `src/bonuses/Lying.cpp` (`LYING_TIME`) |
| change the order of the systems in a frame | `src/game/PlayFrame.cpp` |
| change the caption, the black screens, the end of an attempt | `src/game/phases/`, `src/game/Attempts.cpp` (`end`) |
| change the order of the phases (caption, black screens, play) | `src/game/GameFlow.cpp` (the constructor gives each phase the next one) |
| change how a copter hits walls | `src/physics/CollisionProbe.cpp` |
| change where a copter's door, skids, body or waterline are | `src/world/copter/CopterShape.hpp` |
| change the size of the screen | `src/data/levels/ScreenSize.hpp`: the collision mask, `world::scenery::Screen` (where a thing is gone), the rain (its width and `Rain::DROPS`) follow it; the limits of a copter's flight are its own (`src/physics/CopterPhysics.cpp`) |
| change what ends an attempt (a crash, Esc, the last passenger) | `src/world/Level.cpp` (`crash`, `fadeOut`, `passengerFinished`) |
| change what a frontend gets to draw | `include/ugh_logic.h` (`ugh_logic_view`, the background: `ugh_logic_pad`, `ugh_logic_solid`, the names of the sprites: `ugh_logic_get_sprite`), `src/api/LogicApi.cpp` and `src/api/LevelView.cpp` |
| change how the score multiplier works | `src/world/session/Score.hpp` |
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
