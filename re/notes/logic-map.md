# Mapa `logic/` → Kotlin port → originál

Jediné místo, kde se nové jádro `logic/` (C++) potkává s pamětí a kódem originálu: pro každou třídu a metodu logiky
funkce Kotlin portu (`core/src/main/kotlin/ugh/core/game/`) a adresa rutiny originálu (`113b:xxxx`, segment kódu hry).
V kódu `logic/src` odkazy na originál nejsou (zásady v [rewrite-design.md](rewrite-design.md), kap. 3). Data
(`UGD 1`) a jejich přepočty jsou v `extractor/.../LogicData.kt`, jména v `Names.kt`; sémantický stav replayů
v `verify/.../replay/SemanticProjection.kt`.

## Tok hry, klávesnice, náhoda

| `logic/` | Kotlin port | Originál |
|---|---|---|
| `game::GameFlow::step` | `GameFlow.kt` `playGame` | `113b:0c61 .. 0fe7` |
| `game::Game::startGame` / `world::Session::startGame` | `GameFlow.kt` `newGame` | `113b:3961` |
| `game::phases::BlackScreen` | `Host.kt` `blackPalette` | `113b:4e9b` |
| `game::phases::CaptionFadeIn` (`Game::startAttempt`) | `Level.kt` `levelSetup`, `loadLevel`, `levelCaption`; `Host.kt` `fadeIn` | `113b:3d66`, `3976`, `0664`, `4e19` |
| `game::phases::CaptionWaitKey` | `Host.kt` `waitKey` | `113b:44db` |
| `game::phases::CaptionFadeOut` | `Host.kt` `fadeOut` | `113b:4e28` |
| `game::phases::Playing::enter` (`Game::beforePlay`) | `Level.kt` `levelSetup` (konec), `resetDrawnSprites` | `113b:3d66`, `0b4f` |
| `game::phases::Playing::nextFrame`, `game::PlayFrame::run` | `GameFlow.kt` `playLevel`; `Frame.kt` `frameFade`, `frameBody`, `frameAfterKeys` | `113b:0c7d .. 0fa4`, `0ca5` |
| `game::PlayFrame::readKeys` | `Frame.kt` `frameKeys`, `readScancode` | `113b:0fe8`, `44f1` |
| `game::GameFlow::endAttempt`, `Game::endAttempt` | `GameFlow.kt` `playGame` (po `playLevel`) | `113b:0fa7` |
| `input::PcKeyboard::deliver` | `Host.kt` `keyboardInterrupt` | `113b:4567` |
| `input::PcKeyboard::readLastScancode` | `Frame.kt` `readScancode` | `113b:44f1` |
| `world::RandomNumbers::next` | `Draw.kt` `random` | `113b:4f09` |
| `world::Fade` | `Frame.kt` `frameFade` | `113b:0c7d` |

## Svět levelu a fyzika

| `logic/` | Kotlin port | Originál |
|---|---|---|
| `world::Level::startAttempt` | `Level.kt` `loadLevel`, `levelSetup` (nulování 2648 .. 27cf) | `113b:3976`, `3d66` |
| `world::Level::passengerFinished` | `Passengers.kt` `p149cNextStop` | `113b:149c` |
| `world::Water::move` | `Draw.kt` `updateWater` | `113b:2d1c` |
| `world::Water::animateSurface`, `world::Rain::stopAt` | `Draw.kt` `drawWaterSurface` | `113b:2db9` |
| `world::Rain::move` | `Draw.kt` `moveRain` | `113b:3c78` |
| `world::Rain::spawn` | `Draw.kt` `spawnRaindrop` | `113b:3c35` |
| `world::Rain::start` | `Level.kt` `loadLevel` (konec) | `113b:3976` |
| `world::Copter::spinRotor` | `Draw.kt` `drawCopter` | `113b:418d` |
| `world::Copter::throwUp` | `Objects.kt` `o2748Charging` | `113b:2748` |
| `world::Copter::takeOnBoard` / `pickUpHanging` / `lowerFare` | `Passengers.kt` `p19e0Board` / `p1c48Grabbed` / `p1a42Riding` | `113b:19fb`, `1c48`, `1a42` |
| `world::Energy::refill` | `Bonuses.kt` `b2ca9Lying` | `113b:2ca9` |
| `physics::CopterPhysics::fly` | `Game.kt` `copterUpdate` | `113b:1095` |
| `physics::CopterPhysics::moveHorizontally` | `Game.kt` `moveHorizontally` | `113b:1095` |
| `physics::CopterPhysics::moveVertically`, `bounceVertically`, `touchDownOnPad` | `Game.kt` `moveVertically`, `bounceVertically` | `113b:1095` |
| `physics::CollisionProbe` | `Game.kt` `probe` | `113b:1457` |
| `physics::TouchBox` | `Passengers.kt` `touchesPlayer`; `Objects.kt` `copterTouchesObject`; `Bonuses.kt` `copterTouchesBonus` | `113b:2276`, `22f1`, `2207` |
| `physics::Ballistics` (cestující) | `Passengers.kt` `p1ceeFalling` | `113b:1cee` |
| `physics::Ballistics` (bonus) | `Bonuses.kt` `b2be4Falling` | `113b:2be4` |

## Cestující

| `logic/` | Kotlin port | Originál |
|---|---|---|
| `passengers::Passengers::update` | `Passengers.kt` `passengersUpdate` | `113b:1486` |
| `passengers::Passengers::frameShown` | `Frame.kt` `frameAfterKeys` (kreslení cestujících) | `113b:0ca5` |
| `passengers::Passengers::fallingOnto` | `Objects.kt` `fallingPassengerNear` | `113b:2196` |
| `passengers::PassengerFactory` | `Level.kt` `loadLevel` (seznam B) | `113b:3aaf` |
| `route::NextStop` | `p149cNextStop` | `113b:149c` |
| `route::BehindDoor` | `p1509Arriving` | `113b:1509` |
| `route::ComingOut` (enter / update) | `p153bAppear` / `p1582Appearing` | `113b:153b` / `1582` |
| `route::Waiting` | `p15b4StartWaiting` / `p15d7Waiting` | `113b:15b4` / `15d7` |
| `route::Calling` | `p16f6StartCalling` / `p172aCalling` | `113b:16f6` / `172a` |
| `route::Impatient` | `p17e6StartImpatient` / `p180aImpatient` | `113b:17e6` / `180a` |
| `route::Boarding` | `p18c8StartBoarding` / `p18e6Boarding`; `walkToCopter` | `113b:18c8` / `18e6` |
| `route::Riding` (enter / update) | `p19e0Board` / `p1a42Riding` | `113b:19e0`, `19fb` / `1a42` |
| `route::WalkingToDoor` | `p1a7ePaid` / `p1b29WalkingAway` | `113b:1a7e` / `1b29` |
| `route::GoingIn` | `p1bbeStartEntering` / `p1bd6Entering` | `113b:1bbe` / `1bd6` |
| `route::Gone`, `standing::Gone` | `gone` | `113b:1c0e` |
| `route::OnPickupPad` | `fellIntoWater`, `hitByCopter`, `switchToWaterSet` | `113b:15d7` a dál |
| `route::Splash` | `p1da8StartSplash` / `p1dd5Splash` | `113b:1da8` / `1dd5` |
| `route::Sinking` | `p1e9cStartSinking` / `p1ec0Sinking` | `113b:1e9c` / `1ec0` |
| `route::Swimming` | `p1f24StartSwimming` / `p1f43Swimming`; `onWater`, `floatOnSurface` | `113b:1f24` / `1f43` |
| `route::SwimCalling` / `SwimWaving` / `SwimBoarding` | `p1fe2SwimCalling` / `p2068SwimWaving` / `p20c1SwimBoarding` | `113b:1fe2` / `2068` / `20c1` |
| `standing::Placed`, `standing::Standing` | `p1c0fStartStanding` / `p1c27Standing` | `113b:1c0f` / `1c27` |
| `standing::Hanging` | `p1c48Grabbed` / `p1c6bHanging` | `113b:1c48` / `1c6b` |
| `standing::Falling` | `p1c81Dropped` / `p1ceeFalling` | `113b:1c81` / `1cee` |

## Nepřátelé

| `logic/` | Kotlin port | Originál |
|---|---|---|
| `enemies::Enemies::update` | `Objects.kt` `objectsUpdate` | `113b:2363` |
| `enemies::EnemyFactory` | `Level.kt` `loadLevel` (seznam C) | `113b:3b21` |
| `enemies::Enemy::bounceFallingPassenger` | `passengerHitsObject`, `fallingPassengerNear` | `113b:2196` |
| `flyer::Placed`, `flyer::Hidden` | `o2379FlyerInit` / `o239fFlyerWait` | `113b:2379` / `239f` |
| `flyer::Screeching` | `o23b0FlyerScreech` / `o23d9FlyerWait2` | `113b:23b0` / `23d9` |
| `flyer::Flying` | `o23eaFlyerStart` / `o2493Flying`; `stopFlap` | `113b:23ea` / `2493` |
| `flyer::Falling` | `o252bFlyerHit` / `o255eFlyerFalling` | `113b:252b` / `255e` |
| `walker::Placed`, `walker::Walking` | `o25b1WalkerInit` / `o25c9Walking` | `113b:25b1` / `25c9` |
| `walker::Watching` | `o2667StartWatching` / `o2681Watching`; `faceCopter` | `113b:2667` / `2681` |
| `walker::Charging` | `o272eStartCharging` / `o2748Charging` | `113b:272e` / `2748` |
| `walker::Recovering` | `o2830StartRecovering` / `o2844Recovering` | `113b:2830`, `288f` / `2844` |
| `walker::Stunned` | `o28eeWalkerStunned` / `o2914Stunned` | `113b:28ee` / `2914` |
| `blower::Placed`, `blower::Blowing` | `o295bBlowerInit` / `o2973Blowing` | `113b:295b` / `2973` |
| `blower::Stunned` | `o2a53BlowerStunned` / `o2a76BlowerWait` | `113b:2a53` / `2a76` |
| `tree::Placed`, `tree::Swaying` | `o2a87TreeInit` / `o2ab5Tree` | `113b:2a87` / `2ab5` |
| `tree::Resting` | `o2b0cTreeCatch` / `o2b58TreeWait` | `113b:2b0c` / `2b58` |
| `tree::Bare` | (žádná práce) | `113b:2b7e` |

## Bonusy

| `logic/` | Kotlin port | Originál |
|---|---|---|
| `bonuses::BonusSlots::update` | `Bonuses.kt` `bonusesUpdate` | `113b:2b7f` |
| `bonuses::BonusSlots::drop`, `BonusItem` (konstruktor) | `bonusSpawn` | `113b:2b96` |
| `bonuses::Falling` | `b2be4Falling` | `113b:2be4` |
| `bonuses::Lying` | `b2c97Landed` / `b2ca9Lying` | `113b:2c97` / `2ca9` |
