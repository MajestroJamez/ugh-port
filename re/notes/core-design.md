# C++ jádro: cílový návrh (kroky 9b a 9c)

Stav k 2026-10-03. Krok 9 převedl jádro z paměti DGROUP na typovaný model; logika je bit po bitu správná
(161 replayů), ale kód ještě není takový, aby se v něm junior za chvíli vyznal. Tento dokument popisuje, jak má
jádro vypadat, aby to šlo říct s čistým svědomím. Implementace podle něj, po etapách, s replayi zelenými po každé.

## Co je dnes špatně (výchozí stav, commit f31f76f)

| Problém | Kde | Proč vadí |
|---|---|---|
| 23 souborů naplocho v `src/`, jeden namespace | celé `src/` | nevidět moduly, „kam to patří“ se nedá odhadnout z cesty |
| pravidla jsou volné funkce nad `Game&` s veřejnými poli (`world`, `events`, `problems`) | `copter.cpp`, `water.cpp`, `level.cpp`, `game.hpp` | procedurální kód, `Game` je sklad všeho, žádné zapouzdření |
| entity jsou struktury, ve kterých jedno pole slouží ke dvěma věcem | `Passenger::counter`, `timer`, `Enemy::table`, `facing`, `BonusItem::vx` | čtenář musí znát trik originálu, aby pochopil `p.counter = 1` |
| stavy jsou dvojice {jméno, ukazatel na funkci}, vstupní akce volné funkce (`startCalling` …) | `passengers.cpp`, `enemies.cpp` | není to vzor State, přechody jsou rozházené |
| `PassengerTurn` a `EnemyTurn` mají stejné pomocné metody | `countDown`, `restartAnimation`, `show`, `animationStep` | porušené DRY |
| nepojmenovaná čísla | `0x3f`, `0x1b`, `0x46`, `0x5a`, `0xc1`, `0x15`, `0x175` … | nejde poznat, co je gravitace a co tah |
| `static_cast<int16_t>(…)` skoro na každém řádku | všude | šum, logika se ztrácí |
| šablony, `std::function`, ukazatele na členy, lambdy vracející `auto&` | `replay_projection.cpp` | pro juniora nečitelné |
| korutiny C++20 | `flow.hpp`, `game.cpp` | pro juniora nejtěžší část jazyka |
| „nouzové“ objekty `static Pad nowhere` | `passengers.cpp`, `enemies.cpp` | kontrola vstupu patří na hranici, ne do logiky |
| přehrávač je jeden soubor se 4 třídami | `tools/ugh_replay.cpp` | parser, dvojice jader, porovnání a výpis dohromady |
| testy v jednom souboru, vlastní makro | `tests/unit_tests.cpp` | nedá se najít test k modulu |
| chybí průvodce „chci změnit X → soubor Y“ | — | junior neví, kde začít |

## Zásady

1. **Jedna třída = jeden soubor** (`Copter.hpp` / `Copter.cpp`), jméno souboru = jméno třídy (PascalCase).
   Výjimka: malé pomocné struktury patřící jediné třídě smí být v jejím hlavičkovém souboru.
2. **Složka = modul = namespace** (analogie balíčků v Javě): `src/model/Copter.hpp` → `ugh::model::Copter`.
   Includy vždy s cestou od `src/`: `#include "model/Copter.hpp"`.
3. **Zapouzdření:** stav entity je soukromý; ostatní ji ovládají metodami se slovesem
   (`copter.takeOnBoard(...)`, `copter.throwUp(speed)`, `passenger.boardCopter(...)`), ne zápisem do polí.
4. **Kontrola vstupu na hranici:** továrna dat a adaptér replayů odmítnou neplatná data (index plošiny mimo rozsah,
   neznámý druh). Logika pak smí předpokládat platné hodnoty a nemá nouzové větve.
5. **Žádné magické číslo v logice:** každá konstanta originálu má jméno a jednotku v jménu nebo komentáři
   (`GRAVITY = 0x1b; // 1/64 Fixed per frame²`), u třídy, která ji používá.
6. **Aritmetika originálu jen přes hodnotové typy** (viz níže) – v logice žádné `static_cast<int16_t>`.
7. **Žádné šablony, makra, korutiny, `std::function` ani ukazatele na členy v logice a adaptéru.**
   Dovolené C++: třídy, virtuální metody, `enum class`, `std::array`, `std::vector`, `std::string`, `std::optional`,
   `std::variant` jen tam, kde je hodnota opravdu jedno z několika (a s pojmenovanými přístupovými metodami).
8. **Komentář každé metody** nese adresu originálu a funkci Kotlin portu (`/** 113b:15d7 - Passengers.kt … */`),
   zvláštnosti originálu jsou pojmenované v komentáři u místa, kde se projevují.
9. **DRY:** společné chování stavů (animace, odpočty) je v jedné třídě (`Animator`, `Countdown`), ne v kontextech.

## Struktura

```
sim/
  README.md                      průvodce: moduly, tok snímku, „chci změnit X → soubor Y“, jak spustit testy
  CMakeLists.txt
  include/ugh_sim.h              C API (smlouva pro UE, beze změny chování)
  src/
    core/                        ugh::core - nezávislé stavební kameny
      Word.hpp                   16bit slovo originálu: + - ++ --, posuny, porovnání znaménkové, unsignedLess()
      Fixed.hpp                  poloha v 1/32 px (Word s jednotkou): fromPixels(), pixels(), wholePixel()
      Speed.hpp                  rychlost v 1/64 Fixed: perFrame() -> Fixed
      Countdown.hpp              odpočet originálu (DEC, nula = konec): start(n), tick() -> bool
      Random.hpp/.cpp            generátor 113b:4f09
      Event.hpp                  událost pro frontend (druh, hráč, entita, hodnota)
      EventListener.hpp          rozhraní pozorovatele (Observer)
      Diagnostics.hpp            hlášení situací, které jádro nepodporuje (pauza, plné sloty bonusů)
    data/                        ugh::data - neměnná data hry
      GameData.hpp/.cpp          registr všeho načteného (Repository), dotazy level(), kind() …
      GameDataLoader.hpp/.cpp    továrna (Factory): UGHSIM01 -> objekty, kontroly deskriptorů
      DataImage.hpp              syrová DGROUP pro továrnu (nikdo jiný ji nevidí)
      LevelDefinition.hpp        level: plošiny, umístění cestujících a nepřátel, start, voda, vítr, maska
      PadDefinition.hpp, PassengerPlacement.hpp, EnemyPlacement.hpp
      PassengerKind.hpp, EnemyKind.hpp, BonusKind.hpp, Box.hpp
      Animation.hpp, AnimationPair.hpp, Route.hpp, DropList.hpp
      CollisionMask.hpp/.cpp
      KeyBinding.hpp
    model/                       ugh::model - stav hry a jednoduché chování entit
      GameSession.hpp/.cpp       hra: hráči, obtížnost, číslo levelu, životy, násobič, skóre
      Level.hpp/.cpp             běžící level: entity + dotazy, které stavy potřebují (viz níže)
      Copter.hpp/.cpp            vrtulník: poloha, rychlost, přistání, náklad, jízdné, rotor, ovládání
      Controls.hpp               držené klávesy hráče
      Pad.hpp                    plošina za hry (definice + kdo na ní čeká)
      Passenger.hpp/.cpp         cestující: data + metody, které volají stavy
      Enemy.hpp/.cpp             nepřítel
      BonusItem.hpp/.cpp         bonusový předmět
      BonusSlots.hpp/.cpp        12 slotů, drop() = 113b:2b96
      Water.hpp/.cpp             hladina (113b:2d1c, 2db9)
      Rain.hpp/.cpp              déšť (113b:3c35, 3c78)
      Fade.hpp                   stmívání palety
      Energy.hpp                 společná energie
      Animator.hpp/.cpp          snímek a zpoždění animace (sdílí cestující i nepřátelé)
    physics/                     ugh::physics
      CopterPhysics.hpp/.cpp     let jednoho snímku (113b:1095): vítr, klávesy, gravitace, vztlak, náraz
      CollisionProbe.hpp/.cpp    10 bodů obrysu nad maskou (113b:1457) a zvláštnost „doleva / nahoru jen o pixel“
      TouchBox.hpp/.cpp          dotyk vrtulníku se spritem (113b:2276, 22f1, 2207)
    passengers/                  ugh::passengers - stavový automat (State)
      PassengerState.hpp         základ: name(), enter(), update()
      PassengerStates.hpp        registr všech stavů (adaptér replayů je hledá podle jména)
      walking/NextStop.cpp, Arriving.cpp, Appearing.cpp, Waiting.cpp, Calling.cpp, Impatient.cpp,
              Boarding.cpp, Riding.cpp, WalkingAway.cpp, Entering.cpp, Gone.cpp
      swimming/Splash.cpp, Swimming.cpp, SwimCalling.cpp, SwimWaving.cpp, SwimBoarding.cpp, Sinking.cpp
      standing/StartStanding.cpp, Standing.cpp, Hanging.cpp, Falling.cpp
    enemies/                     ugh::enemies
      EnemyState.hpp, EnemyStates.hpp
      EnemyBehavior.hpp          Strategy: umístění z levelu + počáteční stav
      flyer/FlyerBehavior.cpp + stavy (FlyerInit, FlyerWait, FlyerWait2, Flying, FlyerFalling)
      walker/WalkerBehavior.cpp + stavy (WalkerInit, Walking, Watching, Charging, Recovering, Stunned)
      blower/BlowerBehavior.cpp + stavy (BlowerInit, Blowing, BlowerWait)
      tree/TreeBehavior.cpp + stavy (TreeInit, Tree, TreeWait, Inactive)
    bonuses/                     ugh::bonuses - BonusState, Falling, Lying
    input/                       ugh::input - Keyboard (skenkódy -> Controls)
    game/                        ugh::game
      Game.hpp/.cpp              fasáda: vlastní GameSession, Level, služby, tok; jediný vstup pro API
      GameFlow.hpp/.cpp          tok hry jako stavový automat fází (viz níže), náhrada korutin
      FlowPhase.hpp + phases/    BlackScreen, Caption, Playing, LevelEnd …
      PlayFrame.hpp/.cpp         jeden snímek hry: pořadí systémů (113b:0ca5 .. 0fa4)
      LevelLoader.hpp/.cpp       pokus o level: 113b:3d66 + 3976
    replay/                      ugh::replay - jediné místo se jmény a formáty UGR 0
      ReplayProjection.hpp/.cpp  fasáda: forget(), set(), fields()
      FieldVisitor.hpp           Visitor: signedWord(), unsignedWord(), byte(), hex(), hexOrNone() …
      FieldWriter.cpp, FieldReader.cpp, FieldFiller.cpp   tři návštěvníci: do textu, z textu, výplň
      GameFields.cpp, CopterFields.cpp, PadFields.cpp, PassengerFields.cpp, EnemyFields.cpp, BonusFields.cpp
                                 každý soubor: JEDEN seznam polí entity (visit), sdílený všemi návštěvníky
    api/
      SimApi.cpp                 C API nad Game a ReplayProjection
  tools/ugh_replay/
    main.cpp, ReplayFile.hpp/.cpp (parser), TwinCores.hpp/.cpp, ReplayPlayer.hpp/.cpp, ReplayReport.hpp/.cpp
  tests/
    TestFramework.hpp            TEST(...)/CHECK(...) – jediný soubor s makry, vysvětlený
    core/WordTest.cpp, FixedTest.cpp, RandomTest.cpp
    data/GameDataLoaderTest.cpp
    physics/CollisionProbeTest.cpp, CopterPhysicsTest.cpp
    input/KeyboardTest.cpp
    passengers/WalkingPassengerTest.cpp, SwimmingPassengerTest.cpp, StandingPassengerTest.cpp
    enemies/FlyerTest.cpp, WalkerTest.cpp, BlowerTest.cpp, TreeTest.cpp
    bonuses/BonusTest.cpp
    game/GameFlowTest.cpp
```

## Klíčová rozhodnutí

### Hodnotové typy (`core/`)

- `Word`: obal `int16_t` s operátory, které přetečou jako registr; `<`, `>` znaménkově (JL/JG),
  `unsignedLess(a, b)` pro JB/JA, `>> n` aritmeticky (SAR). Logika pak píše `fare - 1`, ne `static_cast`.
- `Fixed`, `Speed`: jednotky; `Speed::perFrame()` místo `perFrame(int16_t)`.
- `Countdown`: originál všude dělá „DEC, je nula?“; `if (timer.tick())` čte se lépe než `countDown(p.timer)`.
- Bajty originálu (`lives`, `multiplier`, `passengersLeft`) jako `uint8_t` s metodami tam, kde mají pravidlo
  (`lives.lose()` vrací konec hry).

### Model (`model/`) – zapouzdření a Memento

- Každá entita má soukromou strukturu stavu (`Copter::State`) a metody s chováním. Pole jsou v jediné struktuře
  (DRY); metody pracují nad ní.
- **Memento:** `const State& state() const` a `void restore(const State&)` – adaptér replayů čte a zapisuje stav
  jen přes ně. Hodí se i pro UE (uložení hry, přetočení). Nikdo jiný na stav nesahá.
- **Sdílená slova originálu** dostanou malou třídu s pojmenovanými přístupy a jediným syrovým slovem pro replay,
  např. `PassengerCounter`: `startCountdown(n)`, `tick()`, `markAtWaitingSpot()`, `setCarrier(player)`, `carrier()`,
  `word()`. Obdobně `PassengerTimer` (zpoždění příchodu / rychlost po pádu / čas na vodě), `EnemyTable`
  (animace letu / další bonus / zbytek paměti), `EnemyFacing` (směr chodce / cíl ptakoještěra),
  `BonusTimer` (rychlost x / doba ležení). Trik originálu je tak popsaný na jednom místě a logika čte jména.
- `World` se rozdělí na `GameSession` (přežívá levely) a `Level` (běžící level). `Level` nabízí dotazy, které dnes
  dělají kontexty: `copterLandedOn(pad)`, `emptyCopterLandedOn(pad)`, `copterTouching(box, x, y)`,
  `copterOnWater(...)`, `fallingPassengerNear(enemy)`, `pad(i)`, `water()`, `bonuses()`.
- Chování vrtulníku vůči ostatním jako metody: `takeOnBoard(look, targetNumber, fare, fareMin)`, `unload()`,
  `fareTick()`, `throwUp(speed)` (chodec), `push(speedX)` (foukač, vítr), `land(pad)`, `takeOff()`.

### Stavové automaty (`passengers/`, `enemies/`, `bonuses/`) – vzor State

```cpp
class PassengerState {
public:
    virtual ~PassengerState() = default;
    virtual const char* name() const = 0;             // jméno v replayích
    virtual void enter(Passenger&, Level&) const {}   // vstupní akce (dnešní startCalling, appear …)
    virtual void update(Passenger&, Level&) const = 0;
};
```

- Každý stav je bezstavový singleton (`static const Waiting instance;`), jedna třída v jednom souboru.
- Přechody má `Passenger`: `changeState(next)` = `next.enter()` a od příštího snímku `next.update()`;
  `continueIn(next)` = totéž a `update()` ještě v tomto snímku (originál skáče `JMP [SI+n]`). Rozdíl je
  v komentáři u obou metod a je to jediné místo, kde se řeší.
- Dnešní přechodové funkce se stanou vstupními akcemi cílových stavů: `startWaiting` → `Waiting::enter`,
  `startCalling` → `Calling::enter` a `SwimCalling::enter` (společná metoda `Passenger::showDestinationBubble()`),
  `board` → `Passenger::boardCopter(c)` + `changeState(Riding)`, `pay` → `Passenger::payAndLeave()`, …
  Pozor na pořadí zápisů a na to, kdy originál stav nastaví a kdy skočí – replays to hlídají.
- Stavy, které se mezi snímky nikdy neobjeví (Appear, Board, Paid, StartSplash …), nejsou třídami, ale vstupními
  akcemi; registr stavů obsahuje jen ty, co jsou v replayích (seznam v `re/notes/plan.md`, krok 9).
- Nepřátelé: `EnemyBehavior` (Strategy) dělá umístění z levelu a dává počáteční stav; stavy po druzích ve
  složkách `flyer/`, `walker/` … Sdílené chování (náraz padajícího cestujícího, otočení k vrtulníku, animace)
  jako metody `Enemy` / `Animator`, ne kopie.

### Fyzika (`physics/`)

- `CopterPhysics::fly(Copter&, Level&, const GameSession&)` – dnešní `flyCopter` rozdělený na pojmenované kroky
  `applyWind`, `steer`, `applyGravityOrBuoyancy`, `pedal`, `moveHorizontally`, `moveVertically`,
  `bounceOffFloor`, `touchDownOnPad`, `checkCrash`. Konstanty jako `static constexpr` se jménem.
- `CollisionProbe` drží 10 bodů a obě zvláštnosti originálu (sonda doleva a nahoru se neposouvá).

### Tok hry (`game/`) – stavový automat fází místo korutin

Korutiny jsou přesné, ale pro juniora nejtěžší část C++. Nahradí je automat fází (opět vzor State), kde každá
fáze ví, kolik snímků trvá a co udělá po nich. `Game::step()` = jeden snímek originálu = jedna `frame()` aktuální
fáze. Přesný sled dnešních korutin (musí zůstat stejný, replays to hlídají):

```
newGame; 8× retrace (černá paleta)
smyčka:
  LevelLoader.startAttempt (113b:3d66 + 3976)
  popisek: ulož řádek vody, voda = 0xaf, událost LevelCaption; 65× retrace (fade in);
           keyboard.read(); opakuj { retrace; read } dokud se skenkód nezmění;
           obnov řádek vody; 65× retrace (fade out)
  8× retrace (černá paleta); první update nepřátel, pak cestujících; sprity nic / sloty bonusů volné
  hra: opakuj { fade < 0 → konec; retrace; fade += krok, pokud fade <= 0x100; snímek hry }
  konec levelu: další level / ztráta života / konec hry
```

Fáze: `BlackScreen(n, next)`, `CaptionFadeIn`, `CaptionWaitKey`, `CaptionFadeOut`, `Playing`, `GameOver`.
Akce „mezi“ retrace patří do `enter()` následující fáze nebo na konec `frame()` – rozvrhnout přesně podle sledu
výše, snímek po snímku (první run replay `1p-L01-cheat` ukáže každé posunutí o snímek).

### Události (Observer)

`EventListener` s jedinou metodou `onEvent(const Event&)`; `Game` drží seznam posluchačů, C API registruje
frontu (`EventQueue`), testy svého posluchače. `Diagnostics` stejně (hlášení problémů).

### Adaptér replayů (`replay/`) – Visitor

Každá entita má JEDEN seznam polí (`CopterFields::visit(FieldVisitor&, Copter::State&)`), např.
`v.signedWord("xf", s.x); v.hex("sprite", s.rotor); v.keys("keys", s.keys);`. Tři návštěvníci: `FieldWriter`
(do textu), `FieldReader` (nastaví jedno pole z textu), `FieldFiller` (výplň vzorkem). Žádné šablony ani
`std::function`; přidat pole = jeden řádek. Dvojice jader v přehrávači zůstává (je dobrá), jen je popsaná
v `README.md` a `TwinCores.hpp`.

### Průvodce `sim/README.md`

Krátký: moduly a co dělají, tok jednoho snímku (pořadí systémů), jak spustit build a testy, slovníček (Fixed,
retrace, deskriptor, replay), a tabulka „chci změnit X → soubor Y“ (gravitace → `physics/CopterPhysics.cpp`,
jak dlouho cestující volá → `passengers/walking/Calling.cpp`, nový druh nepřítele → `enemies/` + továrna, nové
pole replaye → `replay/…Fields.cpp`, nová událost → `core/Event.hpp` + `ugh_sim.h`).

## Postup (etapy, po každé build + 323 testů zelených)

**Krok 9b** (jedna session):
1. `core/`: `Word`, `Fixed`, `Speed`, `Countdown`, `Random`, `Event`, `EventListener`, `Diagnostics` + jejich testy.
2. Přesun do složek a namespaces (mechanicky), CMake se seznamem souborů po modulech.
3. `data/`: rozdělení na soubory, `GameDataLoader` oddělený od `GameData`, kontrola indexů plošin v továrně.
4. `model/`: entity jako třídy s Mementem, sdílená slova jako malé třídy, `GameSession` + `Level` s dotazy,
   pojmenované konstanty. Adaptér zatím přes Memento upravit jen tolik, aby prošel.
5. `passengers/`, `enemies/`, `bonuses/`: stavy jako třídy s `enter/update`, `Animator`, `EnemyBehavior`.

**Krok 9c** (další session):
6. `physics/`: `CopterPhysics`, `CollisionProbe`, `TouchBox`.
7. `game/`: `GameFlow` jako automat fází, `PlayFrame`, `LevelLoader`; smazat `flow.hpp`.
8. `replay/`: Visitor, jeden seznam polí na entitu.
9. `tools/ugh_replay/` rozdělený, `tests/` po modulech s `TestFramework.hpp`.
10. `sim/README.md`.
11. Kontrola hotového (níže) a `/code-review` na `high`; opravit vše, co se týká čitelnosti.

## Hotovo když

- 161 replayů projde celých i po přechodech se stejným počtem porovnaných hodnot jako dnes; testy jednotek po
  modulech zelené.
- V logice (`model/`, `physics/`, `passengers/`, `enemies/`, `bonuses/`, `game/`, `input/`) není: offset DGROUP,
  `static_cast<int16_t>`, magické číslo bez jména, šablona, makro, korutina, `std::function`, veřejné měnitelné pole
  entity, nouzový `static` objekt.
- Každý soubor má jednu třídu a její jméno; každá metoda má sloveso a odkaz na originál; žádná metoda delší než
  ~40 řádků bez dobrého důvodu.
- Junior test: podle `README.md` jde u pěti vzorových požadavků (změnit gravitaci, prodloužit volání cestujícího,
  přidat událost, přidat pole replaye, přidat druh nepřítele) ukázat jeden nebo dva soubory, kam sáhnout.
- `/code-review` bez nálezů na čitelnost, strukturu a DRY.
