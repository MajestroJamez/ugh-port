# Nové C++ jádro `logic/`: cílový návrh (kroky N1–N8)

Stav k 2026-10-03. Navazuje na review současného jádra `sim/` (kroky 5–9c) a na rozhodnutí z téhož dne:
**napsat úplně novou, uhlazenou C++ aplikaci herní logiky**, která se chová 1:1 jako originál (ověřeno replayi), ale
v kódu nemá nic z paměti staré aplikace. Kroky jsou v [plan.md](plan.md) (sekce „Nové jádro“), tento dokument je
jejich zadání. Každý krok = jedna session; na konci kroku se do sekce „Jak dopadl krok Nx“ (na konci tohoto
dokumentu) zapíšou odchylky od návrhu.

## 1. Cíl

- Nová knihovna herní logiky v C++20 ve složce `logic/`, napsaná od nuly. Fyzika, pravidla, časování a náhoda
  přesně jako originál: **stejné vstupy → stejný stav v každém snímku**, ověřeno na všech golden replayích.
- V C++ kódu **není nic z paměti originálu**: žádné offsety DGROUP, adresy rutin `113b:xxxx`, sdílená slova
  (jedno pole pro tři věci), pevné sloty se zbytky po předchozím levelu, VGA stránky a roviny, souřadnice Amigy,
  hex konstanty místo jednotek, stavy pojmenované podle adres obslužných rutin.
- Všechno, co zná paměť DOSu, je **jen v Kotlinu** (extractor, port, verify). Hranice mezi starým a novým světem
  jsou dva textové soubory, oba čitelné člověkem a nezávislé na jazyce:
  - `UGD 1` – data hry (levely, druhy, animace, klávesy, kolizní masky) s pojmenovanými objekty a hodnotami už
    přepočtenými tak, jak je hra používá,
  - `UGR 1` – golden replay: vstupy po snímcích a **sémantický stav** po každém snímku (pojmy hry, ne adresy).
- Současné `sim/` se nemění a slouží jako čitelná reference (je už napůl sémantické), dokud `logic/` neprojde.
  V kroku N8 se smaže spolu s `UGR 0` a `UGHSIM01`.

```
UGH.EXE ──► extractor (Kotlin) ──► assets/sim/ugh-data.ugd        UGD 1: pojmenovaná data
                                            │
originál v oracle + Kotlin port ──► verify ──► verify/build/replays/ugr1/*.ugr   UGR 1: vstupy + stav po snímcích
                                            │
                       logic/ (C++) ◄───────┘   tools/replay_check: stejné vstupy → stejný stav, pole po poli
```

## 2. Proč ne další úprava `sim/` (výsledek review 2026-10-03)

| Problém v `sim/` | Příklad | Řešení v `logic/` |
|---|---|---|
| Sdílená slova originálu jako třídy | `PassengerCounter` = odpočet / „stojí na místě“ / kdo ho veze; `EnemyFacing` = směr walkera / cíl flyera; `EnemyTable::leftover` | každý význam vlastní pole s vlastním jménem a typem |
| Jedna třída pro čtyři různé nepřátele | `Enemy` s poli, která má jen část druhů (`pad`, `startDelay`, `table`) | `Flyer`, `Walker`, `Blower`, `Tree` jako samostatné třídy |
| Pevné sloty se zbytky po předchozím levelu | `Level` drží 16 cestujících a 5 nepřátel, load přepíše jen část | entity se vytvoří z definice levelu konstruktorem, nic nezbývá |
| Paměť DGROUP za běhu | `DataImage` 64 KiB kvůli animacím, identita = offset (`0x7720`), kontrola adres handlerů v loaderu | data ve formátu UGD 1 se jmény, extractor vše přepočte |
| Jména podle originálu | `FlyerWait2`, `BlowerWait` (je omráčený), `TreeWait`, `object.N` | jména podle chování (`Screeching`, `Stunned`, `Resting`, `enemy.N`) |
| Artefakty vykreslování | řádek vody `0xaf` během popisku, déšť jako index ve VGA stránce, `hideSprites()` | nic z toho; zvláštnosti s vlivem na hru jako pojmenovaná pravidla (kap. 6) |
| Doména závisí na C API | `core/Event.hpp` a `Game` používají `UGH_SIM_*` | vlastní `enum class`, převod jen v `api/` |
| DRY | balistický pád dvakrát (cestující, bonus), okraje obrazovky ve 4 souborech, `WIND_TO_THE_LEFT` dvakrát | `physics::Ballistics`, `world::Screen`, `enum class Wind` |
| Balíčky | „složka = namespace“ neplatí v podsložkách, dvě třídy `Falling` | složka = namespace všude |
| Zadní vrátka | `restore(Snapshot)` obchází zapouzdření, logika má obranné kontroly kvůli replayům | žádný zápis stavu zvenku kromě `Cheats` (kap. 9) |
| Pasti pro juniora | `GameFlow` konstruuje fáze s referencemi na ještě nezkonstruované členy | fáze přes `PhaseId`, žádné závislosti na pořadí konstrukce |
| Nový stav = 4 místa | hpp, cpp, ruční registr stavů, `CMakeLists` | bez registru (replay se jen zapisuje), CMake po modulech |

## 3. Zásady kódu `logic/`

1. **Jedna třída = jeden soubor**, jméno souboru = jméno třídy (PascalCase). Malá pomocná struktura patřící jediné
   třídě smí být v jejím hlavičkovém souboru.
2. **Složka = modul = namespace, bez výjimky** (i podsložky): `src/enemies/walker/Charging.hpp` je
   `ugh::enemies::walker::Charging`. Includy vždy od `src/`.
3. **Žádná paměť originálu:** v `logic/src` se nesmí objevit `DGROUP`, `113b`, `CS:`, offset, adresa, VGA, Amiga.
   Odkazy na originál a Kotlin port jsou jen v jednom dokumentu `re/notes/logic-map.md` (třída/metoda → funkce
   Kotlin portu → adresa originálu). Komentáře v kódu popisují chování, ne instrukce.
4. **Čísla s jednotkami:** desítkově, s jednotkou v typu nebo jménu (`Fixed::fromPixels(304)`, `Speed(27)  // 1/64
   Fixed za snímek²`, `constexpr int CALL_TIME_FRAMES = 140`). Hex jen pro bitové masky. Žádné magické číslo
   v logice; čísla spritů přicházejí z dat (`SpriteIds`), ne z kódu.
5. **Aritmetika originálu přes hodnotové typy** (`Int16`, `Fixed`, `Speed`, `Countdown`): 16bitové přetečení,
   aritmetické posuny a znaménkové / neznaménkové porovnání jsou vidět v typu a jménu metody, ne v castech.
6. **Zapouzdření:** stav entity je soukromý, mění se jen metodami se slovesem (`copter.takeOnBoard(...)`). Ven jen
   čtecí metody (renderer, zápis replaye). Žádný `Snapshot` / `restore`.
7. **Žádné nemožné stavy:** chybějící hodnota je `std::optional` (`landedPad`, `waitingPassenger`, `carrier`), ne
   `-1`; výčet je `enum class` (`Wind`, `Difficulty`, `Facing`, `BonusEffect`, `PlayerKey`). Logika nemá obranné
   kontroly – vstup kontroluje jen loader dat.
8. **Povolené C++:** třídy, virtuální metody, `enum class`, `std::array`, `std::vector`, `std::unique_ptr`,
   `std::optional`, `std::string`, `std::string_view`. **Zakázané:** šablony (vlastní), makra (kromě
   `tests/TestFramework.hpp`), korutiny, `std::function`, ukazatele na členy, **výjimky a RTTI** (`dynamic_cast`,
   `typeid`) – Unreal Engine je ve výchozím stavu nemá; chyby vrací `std::optional` / struktura s textem chyby.
9. **Závislosti jen jedním směrem:** `units` ← `data` ← `events` ← `world` ← (`physics`, `passengers`, `enemies`,
   `bonuses`) ← `input` ← `game` ← `api`. `replay/` a `tools/` jen čtou `game`. Nic v `src/` neincluduje
   `include/ugh_logic.h` kromě `api/`.
10. **Metody krátké** (~30 řádků), pojmenované slovesem; každá třída a veřejná metoda má jednořádkový komentář,
    co dělá v pojmech hry.
11. **DRY:** společné chování je v jedné třídě (`Animator`, `Ballistics`, `Screen`, `TouchBox`), ne v kopiích.

## 4. Moduly a třídy

```
logic/
  README.md                 průvodce: moduly, tok snímku, „chci změnit X → soubor Y“, build a testy, slovníček
  CMakeLists.txt            knihovny ugh_logic (hra), ugh_logic_replay (zápis UGR 1), nástroj replay_check, testy
  build.ps1                 jako sim/build.ps1 (VS Build Tools, Ninja, CTest)
  include/ugh_logic.h       C API pro Unreal Engine (kap. 10)
  src/
    units/                  ugh::units – hodnotové typy originálu
      Int16.hpp             16bitové celé číslo, které přetéká jako registr; < > znaménkově, unsignedLess()
      Fixed.hpp             poloha / vzdálenost v 1/32 px: fromPixels(), pixels(), wholePixel()
      Speed.hpp             rychlost v 1/64 Fixed za snímek: perFrame(), clamped()
      Countdown.hpp         odpočet „sniž, je nula?“: start(n), tick() -> bool
    data/                   ugh::data – neměnná data hry (Repository) a jejich načtení
      GameData.hpp/.cpp     vše načtené: levely v pořadí obou režimů, druhy, pravidla, klávesy, sprity
      DataFileReader.hpp/.cpp   parser UGD 1 + kontroly (jména existují, indexy plošin platí, trasy mají zastávku)
      Rules.hpp             limity podle obtížnosti (náraz, násobič), bonus za rychlé doručení
      SpriteIds.hpp         pojmenované sprity (stojící / padající / odražený cestující, bubliny, rotor ...)
      Animation.hpp         snímky (sprity) animace (+ případné „přetečení“, viz kap. 6)
      AnimationPair.hpp     varianta doleva a doprava
      Box.hpp               kotva a poloviční rozměr dotykového obdélníku spritu
      PassengerKind.hpp     druh cestujícího (typ, animace, jízdné, čas na vodě, vzhled, protějšek na vodě)
      FlyerKind.hpp, WalkerKind.hpp, BlowerKind.hpp, TreeKind.hpp   druhy nepřátel, každý jen se svými poli
      BonusKind.hpp         druh bonusu (efekt, množství, výskok, sprite, kotva)
      Route.hpp             trasa cestujícího: zastávky (plošina, zpoždění ve snímcích)
      LevelDefinition.hpp   level: plošiny, umístění, start vrtulníků, voda, vítr, maska
      PadDefinition.hpp
      RoutePassengerPlacement.hpp, StandingPassengerPlacement.hpp
      FlyerPlacement.hpp, WalkerPlacement.hpp, BlowerPlacement.hpp, TreePlacement.hpp
      CollisionMask.hpp/.cpp    384 × 192 px; solid(x, y) včetně přetékání řádků (kap. 6)
      KeyBinding.hpp        sekvence skenkódů → hráč, klávesa, stisk / uvolnění
    events/                 ugh::events
      EventKind.hpp, Event.hpp, EventListener.hpp, EventQueue.hpp, EventBroadcast.hpp
      Diagnostics.hpp       situace, které jádro nepodporuje (pauza, 12 bonusů naráz)
    world/                  ugh::world – stav rozehrané hry
      Session.hpp/.cpp      hráči, obtížnost, číslo levelu, životy, násobič, skóre, generátor náhody
      RandomNumbers.hpp/.cpp    generátor originálu (4 slova se sčítáním s přenosem)
      Level.hpp/.cpp        běžící pokus: vrtulníky, plošiny, cestující, nepřátelé, bonusy, voda, déšť, energie,
                            fade; dotazy (který vrtulník přistál na plošině, který plave na vodě ...)
      PlayContext.hpp       co dostane každý update: Level&, Session&, const GameData&, EventListener&, Diagnostics&
      Copter.hpp/.cpp, Cargo.hpp   vrtulník; náklad (kdo, kam, za kolik)
      Controls.hpp          držené klávesy hráče
      Pad.hpp               plošina za hry (definice + kdo na ní čeká)
      Water.hpp/.cpp, Rain.hpp/.cpp, Raindrop.hpp, Energy.hpp, Fade.hpp
      Animator.hpp          snímek animace a odpočet do dalšího (cestující i nepřátelé)
      Screen.hpp            okraje hrací plochy (kdy předmět „odletí z obrazovky“)
      Wind.hpp, Difficulty.hpp, Facing.hpp
    physics/                ugh::physics
      CopterPhysics.hpp/.cpp    let jednoho snímku v pojmenovaných krocích
      CollisionProbe.hpp/.cpp   10 bodů obrysu nad maskou, zvláštnost „doleva / nahoru jen o pixel“
      TouchBox.hpp/.cpp     dotyk vrtulníku se spritem
      Ballistics.hpp/.cpp   pád předmětu (padající cestující, bonus): pohyb, gravitace, okraje, dopad na plošinu
    passengers/             ugh::passengers
      Passenger.hpp/.cpp    základ: pořadí v seznamu, poloha, „poloha z minulého snímku“, sprite, bublina, Animator
      route/                ugh::passengers::route – cestující s trasou (chodí, jezdí, padá do vody, plave)
        RoutePassenger.hpp/.cpp, RouteState.hpp
        NextStop, BehindDoor, ComingOut, Waiting, Calling, Impatient, Boarding, Riding, WalkingToDoor, GoingIn, Gone,
        Splash, Swimming, SwimCalling, SwimWaving, SwimBoarding, Sinking   (.hpp/.cpp každý)
        OnPickupPad.hpp/.cpp      společné kontroly stavů na plošině (voda po kolena, náraz vrtulníkem)
      standing/             ugh::passengers::standing – cestující, který čeká na plošině na vyzvednutí kamkoli
        StandingPassenger.hpp/.cpp, StandingState.hpp
        Placed, Standing, Hanging, Falling, Gone
    enemies/                ugh::enemies
      Enemy.hpp/.cpp        základ: pořadí v seznamu, poloha, sprite, Animator; update()
      EnemyFactory.hpp/.cpp vytvoří správnou třídu z umístění v levelu (Factory)
      flyer/                ugh::enemies::flyer – Flyer, FlyerState, Placed, Hidden, Screeching, Flying, Falling
      walker/               ugh::enemies::walker – Walker, WalkerState, Placed, Walking, Watching, Charging,
                            Recovering, Stunned
      blower/               ugh::enemies::blower – Blower, BlowerState, Placed, Blowing, Stunned
      tree/                 ugh::enemies::tree – Tree, TreeState, Placed, Swaying, Resting, Bare
    bonuses/                ugh::bonuses – BonusItem, BonusSlots (12, volný slot od konce), BonusState, Falling, Lying
    input/                  ugh::input – PcKeyboard: skenkódy PC klávesnice (i E0 páry) → Controls hráčů, poslední
                            skenkód pro popisek a Esc / P
    game/                   ugh::game
      Game.hpp/.cpp         fasáda: newGame(settings), scancode(code), step() -> GameResult, čtení stavu, cheats()
      NewGameSettings.hpp   hráči, obtížnost, první level, stav generátoru, řádek deště (kap. 6)
      GameResult.hpp        Continue, GameOver, AllLevelsDone
      GameFlow.hpp/.cpp, PhaseId.hpp, Phase.hpp
      phases/               ugh::game::phases – BlackScreen, CaptionFadeIn, CaptionWaitKey, CaptionFadeOut, Playing
      PlayFrame.hpp/.cpp    jeden snímek hry: pořadí systémů
      LevelLoader.hpp/.cpp  nový pokus o level z jeho definice
      Cheats.hpp/.cpp       zásahy testovacího pilota (kap. 9): přemístit vrtulník, energie, životy
    api/
      LogicApi.cpp          C API nad Game
  replay/                   ugh::replay – knihovna ugh_logic_replay (jen pro nástroj a testy, ne pro UE)
    StateWriter.hpp/.cpp    sémantický stav hry jako pole UGR 1 (jen čte)
    GameFields, CopterFields, PadFields, PassengerFields, EnemyFields, BonusFields (.hpp/.cpp)
  tools/replay_check/       ugh::tool – ReplayFile (parser UGR 1), ReplayCheck, ReplayReport, main.cpp
  tests/                    po modulech (units/, data/, world/, physics/, passengers/, enemies/, bonuses/, input/, game/)
```

## 5. Klíčová rozhodnutí návrhu

### Entity podle druhu (polymorfismus + State)

- `Passenger` je základ s tím, co mají všichni (pořadí v seznamu B, poloha, poloha z minulého snímku, sprite,
  bublina, `Animator`, `virtual void update(PlayContext&)`). `RoutePassenger` a `StandingPassenger` jsou potomci,
  každý se svými poli a vlastním stavovým automatem.
- Cestující s trasou, který spadne do vody, **zůstává stejný objekt** (stejné pořadí, stejná trasa); jen přepne druh
  na svůj vodní protějšek (`PassengerKind::waterKind`) a jeho stavy jsou vodní. Proto jsou vodní stavy v `route/`.
- `Enemy` je základ, `Flyer`, `Walker`, `Blower`, `Tree` jsou potomci, `EnemyFactory` je vytvoří z umístění.
- `Level` drží seznam cestujících a seznam nepřátel **v pořadí z definice levelu** (`std::vector<std::unique_ptr<…>>`)
  – na pořadí updatů záleží. Kde stav potřebuje konkrétního potomka (nepřítel odrazí padajícího stojícího
  cestujícího), `Level` vede i typovaný pohled (`std::vector<StandingPassenger*>`) – bez RTTI.
- Stavy: vzor State, bezstavové objekty (`static const Waiting instance`), `enter()` a `update()`. Přechod
  `changeState(next)` (vstupní akce teď, update od příštího snímku) a `continueIn(next)` (obojí teď) – rozdíl je
  popsaný na jednom místě v základní třídě entity. Registr stavů není potřeba (replay se nečte zpět do stavu).
- Stavy „Placed“ (dnes `*Init`, `StartStanding`) zůstávají: je to skutečný stav „umístěn, ještě nezačal“, který je
  vidět na popisku levelu, než první update entitu rozběhne. Jinak žádné průchozí stavy navíc.

### Kontext updatu (Parameter Object)

`PlayContext` nese `Level&`, `Session&`, `const GameData&`, `EventListener&`, `Diagnostics&`. Stav dostane kontext
a entitu (`update(Walker&, PlayContext&)`); `Level` tím přestane být skladem všeho a je jen „co je v levelu
a dotazy na to“.

### Rozdělená sdílená slova (nejdůležitější změna modelu)

| `sim/` (slovo originálu) | `logic/` (pole s jedním významem) |
|---|---|
| `PassengerCounter` | `RoutePassenger::callTime` (Countdown), `waitingSpot` (enum Starting/Walking/Reached), `carrier` (optional hráč); `StandingPassenger::carrier` |
| `PassengerTimer` | `RoutePassenger::arrivalDelay`, `swimTime`; `StandingPassenger::dropSpeedX` |
| `Passenger::vy` | `StandingPassenger::fallSpeed` (Int16, 1/32 px), `RoutePassenger::swimSpeed` (Speed) |
| `Passenger::bonusTimer` | `RoutePassenger::quickDeliveryTime` |
| `EnemyTimer` | `Flyer::waitTime`, `screechTime`, `fallSpeed`; `Walker::watchTime`, `chargeSpeed`, `stunTime`; `Blower::stunTime`; `Tree::restTime` |
| `EnemyFacing` | `Walker::facing` (enum Facing); `Flyer::lastTarget` (hráč) |
| `EnemyTable` | `Flyer::flight` (AnimationPair strana), `Tree::nextDrop` (index v seznamu bonusů) |
| `BonusTimer` | `BonusItem::speedX` (Falling), `lyingTime` (Lying) |
| `Copter::carrying/targetPad/fare/fareMin` | `std::optional<Cargo>` (vzhled cestujícího, cíl k zobrazení nebo „visí“, jízdné, minimum) |

Krok N1 ověří, že originál nikdy nečte slovo v jiném významu, než v jakém ho naposledy zapsal. Pokud ano, je to
zvláštnost s vlivem na hru a dostane jméno (kap. 6).

### DRY ve fyzice

- `physics::Ballistics` – jeden pohyb padajícího předmětu (x s okraji, gravitace, y, dolní okraj, dopad na plošinu)
  s parametry: gravitace, okraje, pravidlo dopadu. Pozor na rozdíly, které musí zůstat: bonus dopadne jen tehdy,
  když byl předtím **nad** povrchem (cestující i když byl **na** něm), a jeho střed smí být o pixel vpravo za
  plošinou.
- `world::Screen` – okraje hrací plochy v pixelech (vlevo −16, vpravo 320, dole 192; flyer vlevo −32) jedno místo.

### Tok hry bez pastí

`GameFlow` vlastní všechny fáze, fáze mezi sebou přechází přes `flow.goTo(PhaseId::CaptionFadeOut)`. Žádné reference
na fáze v konstruktorech. `GameResult` je `enum class`, převod na čísla C API jen v `api/`.

### Vstup

`input::PcKeyboard` je adaptér „skenkódy PC klávesnice → `Controls` hráčů + poslední skenkód“ (tabulka kláves
z dat). Logika zná jen `Controls` a dotaz „změnil se poslední skenkód“ (popisek čeká na klávesu, Esc vzdá hru,
P = nepodporovaná pauza). Replay nese skenkódy, protože to je skutečný záznam vstupu; Unreal později dodá vlastní
adaptér (UE vstup → `Controls`).

### Bez výjimek a RTTI

Loader vrací `std::optional<GameData>` a text chyby; žádná část `src/` nehází výjimky. Kód se tak bez úprav přeloží
jako modul Unreal Engine (krok 10).

## 6. Zvláštnosti originálu, které zůstávají (pojmenované chování)

Tohle **není** paměť – je to chování, které hráč pozná, nebo které mění další průběh (náhoda, časování).
Každá zvláštnost má v kódu jméno a komentář u místa, kde se projevuje. Seznam ověřil krok N1 (počty výskytů
v [rewrite-audit.md](rewrite-audit.md), kap. 5).

| Zvláštnost | Kde v `logic/` | Jak |
|---|---|---|
| 16bitové přetečení poloh a rychlostí | `units/` | `Int16`, `Fixed`, `Speed` přetékají (v replayích se nestane, typ zůstává kvůli věrnosti) |
| Sonda doleva / nahoru zkouší jen pixel vedle vrtulníku | `physics/CollisionProbe` | pojmenovaná větev, komentář „rychlý vrtulník proletí tenkou zdí doleva / nahoru“ (N1: 25× doleva, 242× nahoru) |
| Mimo masku nic není pevné | `data/CollisionMask::solid(x, y)` | maska 320 × 192; „bod za okrajem čte sousední řádek“ zaniká (sloupce 320..383 jsou v originálu vždy prázdné, N1) |
| Dotyk se spritem porovnává jen levý horní roh vrtulníku | `physics/TouchBox` | obdélník zvětšený o tělo vrtulníku |
| Cestující s trasou se rozhodují podle polohy viděné minulý snímek | `passengers/route/RoutePassenger::seen` | obnovuje se na konci snímku, jen když je vidět; skrytý ji drží – první krok ve WalkingAway porovná dveře s polohou viděnou při nástupu |
| Rotor se točí i při stmívání, kdy vrtulníky stojí | `world/Copter::effort` | `effort` posledního snímku fyziky přežívá do dalšího pokusu; nová hra ho nuluje |
| Nástup (i po záchraně z vody) uvolní plošinu vyzvednutí | `passengers/route/Riding` | i když na ní mezitím čeká jiný cestující |
| Před prvním snímkem hry proběhne jeden update nepřátel, pak cestujících, a vše se skryje | `game/phases/Playing` | pořadí: nepřátelé, cestující |
| Vrtulník odhozený walkerem má pixelové y o snímek pozadu | `world/Copter::throwUp` | pojmenované |
| Voda se hýbe každý druhý snímek a po změně řádku jeden snímek stojí | `world/Water` | `evenFrame`, `resting` |
| Déšť: 193 kapek, sudé 3 px, liché 2 px za snímek, kapka za okrajem stránky 384 px pokračuje na dalším řádku | `world/Rain`, `world/Raindrop` | kapka jako (x, y) s pojmenovaným přetečením řádku (řádek při zrodu je vždy < 192, ořez na bajt zaniká) |
| Kapky začínají znovu od řádku **poslední vykreslené hladiny** a ten řádek přežívá mezi pokusy; před větrným levelem padá déšť 577 snímků naprázdno | `world/Rain::floorRow`, `NewGameSettings::rainFloorRow` | pravidlo „déšť si pamatuje poslední hladinu“; na začátku hry ho dodá nastavení (hodnota z obrazovky před hrou) |
| Náhoda: generátor ze 4 slov, stav na začátku hry je vstup | `world/RandomNumbers`, `NewGameSettings::randomSeed` | |
| Životy: Esc je vynuluje, ztráta pod nulu = konec hry; zbývající cestující nejdou pod nulu | `world/Session`, `world/Level` | pojmenovaná pravidla, ne `& 0x80` |
| Neznaménková porovnání (energie do plna, jízdné do minima, bublina cíle, dno při potápění) | na místě | `Int16::unsignedLess` se jménem pravidla |
| Skóre za doručení = jízdné × násobič (32 bitů) | `passengers/route/WalkingToDoor` | |
| Animace: pozice snímku zůstává při přepnutí na jinou animaci | `world/Animator` | za koncem seznamu se nikdy nečte (N1), UGD 1 bez přetečení |
| Vrtulníky stojí, dokud fade-in nedojde na tři čtvrtiny; pokus končí, až fade-out dojde do černé | `world/Fade` | |
| Časování fází: 8 snímků černé, 65 fade popisku, čekání na změnu skenkódu, 65, 8 | `game/phases` | |
| 12 bonusů naráz: originál spadne | `bonuses/BonusSlots` | `Diagnostics` |
| Pauza P | `game/PlayFrame` | `Diagnostics` (nepodporováno) |

## 7. Co se zahazuje (nebude v `logic/` ani v UGR 1)

- `DataImage`, offsety a identity podle offsetů, kontrola adres handlerů v deskriptorech, převody Amigy (udělá
  extractor), limity polí originálu (10 plošin, 16 cestujících, 5 nepřátel – hru neovlivňují).
- Zbytky ve slotech, paměť z dema na úvodní obrazovce, výplňové vzorky, dvojice jader (`TwinCores`),
  `ugh_sim_clear/reset(fill)`, „převzaté“ hodnoty.
- Řádek vody `0xaf` během popisku (popisek si ho jen půjčuje na kreslení; nic z logiky ho nečte).
- `hideSprites()` a vše „co se kreslilo minule“, stavový řádek.
- Režim „každý přechod zvlášť“ a s ním zápis libovolného pole zvenku (`ugh_sim_set`), řádky `B` replayů,
  `ugh_sim_has_stage`.
- Mrtvá pole: `passenger.startPad` (nikdo ho nečte), konstanty umístění nepřátel v replayi (`pad`, `startDelay` –
  jsou v datech).

## 8. Formát dat UGD 1 (`assets/sim/ugh-data.ugd`)

Text ASCII, jeden záznam na řádek: `<typ> <jméno nebo pozice> <klíč>=<hodnota> ...`, `#` komentář. Hodnoty desítkově,
seznamy čárkou, rozsahy `a..b`. Všechny hodnoty jsou **už přepočtené** tak, jak je hra používá (pixely PC, snímky
70 Hz, `Fixed` v 1/32 px jako celé číslo). Vytváří ho `extractor/src/main/kotlin/ugh/extractor/LogicData.kt`, jména
dává jediná tabulka `extractor/src/main/kotlin/ugh/extractor/Names.kt` (offset deskriptoru → jméno; tu sdílí i
sémantická projekce replayů).

```
UGD 1
# UGH! game data for the C++ logic (extracted from UGH.EXE sha256 ef93d2cd...)
rules crashLimit=<easy>,<medium>,<hard> multiplierLimit=<e>,<m>,<h> quickDeliveryBonus=<bonus>
sprites standingPassenger=544 droppedPassenger=545 bouncedPassenger=546 shakenTree=232 destinationBubbles=268..273 impatientBubble=274 rotor0=<a>..<b> rotor1=<c>..<d>
key codes=224,72 player=0 key=up press=1
key codes=42 key=none
animation <jméno> frames=<sprite>,<sprite>,...
passengerKind <jméno> type=route|water|standing box=<x>,<y>,<halfW>,<halfH> standing=<anim> waving=<anim> walk=<animL>,<animR> comingOut=<anim> goingIn=<anim> animDelay=<snímky> fare=<n> fareMin=<n> swimTime=<snímky> look=<n> waterKind=<jméno> rescuable=0|1
flyerKind box=... flight=<animL>,<animR> hit=<animL>,<animR> score=<n>
walkerKind box=... walk=<L>,<R> watch=<L>,<R> charge=<L>,<R> recover=<L>,<R> stunned=<L>,<R> score=<n>
blowerKind box=... blowing=<anim> stunnedSprite=<sprite> score=<n>
treeKind box=... swaying=<anim>
bonusKind <jméno> effect=energy|life|multiplier amount=<n> lift=<1/32 px za snímek> sprite=<n> anchor=<x>,<y>
level <id> toDeliver=<n> wind=none|left|right start0=<xFixed>,<yFixed> start1=<xFixed>,<yFixed> water=<Fixed> waterSpeed=<Fixed za 2 snímky>
pad left=<px> right=<px> y=<px> door=<px> wait=<px> stand=<px> number=<n>
routePassenger kind=<jméno> pad=<i> route=<pad>/<zpoždění ve snímcích>,<pad>/<zpoždění>,...,<pad>
standingPassenger pad=<i> x=<Fixed> y=<Fixed>
flyer startDelay=<snímky> speed=<Fixed za snímek>
walker pad=<i> x=<Fixed> y=<Fixed> speed=<Fixed za snímek>
blower x=<Fixed> y=<Fixed>
tree pad=<i> x=<Fixed> y=<Fixed> drops=<bonus>,<bonus>,...
mask <96 hex číslic = 384 px, nejvyšší bit vlevo>          (192 řádků)
order oneplayer=<id>,<id>,...
order team=<id>,<id>,...
```

- Řádky `pad`, `routePassenger` ... `mask` patří k poslednímu `level` nad nimi (v pořadí seznamů originálu; na pořadí
  záleží). `<id>` levelu je jeho pořadí v `levels.json`.
- Jména druhů cestujících podle vzhledu (N2 projde `assets/sprites/*.png`; když vzhled nejde poznat, `kind1`...),
  vodní protějšek `<jméno>-water`, stojící `standing`. Bonusy podle efektu a množství (`energy-small` ...).
- Loader v C++ kontroluje: hlavička, známé typy záznamů a klíče, odkazy na jména existují, indexy plošin v rozsahu
  levelu, trasa má aspoň jednu zastávku, maska 192 × 96 číslic, obě pořadí odkazují na existující levely.

## 9. Formát replayů UGR 1 (`verify/build/replays/ugr1/*.ugr`)

```
UGR 1
# UGH! golden replay - semantic state after every frame
meta exe-sha256=... level=<0..> players=1|2 difficulty=easy|medium|hard pilot=random-keys|cheat-pilot seed=<n>
T <tick> k=<skenkódy hex, čárkou, nebo -> | <pole>=<hodnota> ...
I <pole>=<hodnota> ...
```

- `T` řádek = stav **po** snímku; jen pole, která se změnila (`~` = pole zmizelo); první `T` (tick 0, fáze `start`)
  je celý. `k=` jsou skenkódy doručené **po** snímku (vstup dalšího snímku). Bez `w=` (adresa) a bez řádků `B`.
- `I` řádek = zásah testovacího pilota mezi snímky. Smí obsahovat jen `copter.N.x/y/pixelX/pixelY/vx/vy/landedPad`,
  `game.energy`, `game.lives`; nástroj je převede na `Cheats` (nic jiného zvenku nastavit nejde).
- Hodnoty: celá čísla desítkově (polohy v 1/32 px, rychlosti v 1/64 z toho, sprity = čísla `assets/sprites/NNN.png`),
  jména pro výčty, stavy a druhy, `none` pro chybějící hodnotu, `0/1` pro ano/ne.
- **Pravidlo úplnosti:** pole je ve stavu, právě když je definované – buď ovlivňuje budoucí průběh (stav entity ho
  zapsal a později ho čte), nebo ho potřebuje renderer (viditelná entita: poloha, sprite, bublina, animace).
  Hodnoty, které originál drží jen jako zbytek paměti, v replayi **nejsou**. Pravidla jsou v Kotlinu
  (`SemanticProjection.kt`) jako tabulka po stavech a stejná pravidla má C++ `StateWriter`. Nástroj vyžaduje
  **stejnou množinu polí** i stejné hodnoty – tím se hlídá i shoda pravidel.

### Pole (ověřeno v N1 otrávením nedefinovaných polí; N3 doplní finální tabulku sem)

**game** (vždy): `phase` (`start`, `betweenLevels`, `caption`, `setup`, `play` – N3 sjednotí s fázemi C++), `level`
(pořadí v režimu od 0), `players`, `difficulty`, `lives`, `multiplier`, `score`, `rng` (16 hex číslic = 4 slova
generátoru, poslední první), `rainFloor` (řádek). **V `caption` a `play`:** `energy`, `fade` (0..256),
`fadeDirection` (`in`/`out`), `levelDone`, `wind`, `passengersLeft`, `water.level` (Fixed), `water.resting`,
`water.evenFrame`, `water.surfaceFrame`, `water.surfaceDelay`, `rain` (`none` bez větru, jinak CRC-32 kapek jako
dvojic int16 LE `x, y` v pořadí 0..192, 8 hex číslic). Totéž v `setup`. `water.row` není (= `water.level >> 5`, N1).

**copter.N** (N < hráči; `caption`, `setup`, `play`): `x`, `y`, `pixelX`, `pixelY`, `vx`, `vy`, `landedPad` (index /
`none`), `rotorSprite`, `rotorCounter`, `keys` (`UDLRF` / `-`), `cargoLook` (`none` / číslo), `destination` (číslo
plošiny, `hanging`, `none`), `fare`, `effort`. Jen s cestujícím s trasou na palubě: `fareMin`. `impact` není (pomocná
hodnota jednoho snímku fyziky, N1).

**pad.N**: `left`, `right`, `y`, `door`, `wait`, `stand`, `number`, `waiting` (index cestujícího / `none`).

**passenger.N** (všichni): `kind`, `state`, `sprite`, `bubble`. Poloha `x`, `y`: všechny stavy kromě NextStop,
BehindDoor, Riding, Hanging, Gone. Cestující s trasou: `routeStop` (kromě Gone), `pickupPad`, `targetPad` (pohled na
zastávku; kromě NextStop a Gone), `seenX`, `seenY` (poloha viděná minulý snímek; od ComingOut po GoingIn, ve vodě
i v Riding). Animující stavy: `animFrame`, `animDelay`. Podle stavu: `arrivalDelay` (BehindDoor), `callTime`
(Calling, Impatient, SwimCalling, SwimWaving), `waitingSpot` (Waiting: `starting`/`walking`/`reached`), `carrier`
(Riding; stojící: Hanging), `quickDeliveryTime` (Riding), `dropSpeedX` a `fallSpeed` (stojící: Falling), `swimSpeed`
(Splash, Sinking), `swimTime` (Swimming).

**enemy.N**: `kind` (`flyer`/`walker`/`blower`/`tree`), `state`; `sprite` ve všech stavech kromě Placed (tam drží
originál zbytek paměti, N1); `x`, `y` (flyer jen ve Flying a Falling), a když stav animuje nebo animace pokračuje
v dalším stavu: `animFrame`, `animDelay` (flyer: Hidden, Screeching, Flying; walker: všechny kromě Placed; blower:
Blowing; tree: Swaying, Resting). Flyer: `vx`, `lastTarget`, v Flying `flight` (`left`/`right`), v Hidden
`waitTime`, v Screeching `screechTime`, ve Falling `fallSpeed`. Walker: `vx`, `facing`, ve Watching `watchTime`,
v Charging `chargeSpeed`, ve Stunned `stunTime`. Blower: ve Stunned `stunTime`. Tree: `nextDrop`, v Resting
`restTime`.

**bonus.N** (slot 0..11, jen obsazený): `kind`, `state`, `x`, `y`, `sprite`; ve Falling `vx`, `vy`, v Lying
`lyingTime`.

### Jména stavů (UGR 0 → UGR 1 = třída v `logic/`)

| UGR 0 | UGR 1 | | UGR 0 | UGR 1 |
|---|---|---|---|---|
| passenger NextStop | NextStop | | object FlyerInit | flyer Placed |
| Arriving | BehindDoor | | FlyerWait | Hidden |
| Appearing | ComingOut | | FlyerWait2 | Screeching |
| Waiting, Calling, Impatient, Boarding, Riding | beze změny | | Flying | Flying |
| WalkingAway | WalkingToDoor | | FlyerFalling | Falling |
| Entering | GoingIn | | WalkerInit | walker Placed |
| Gone | Gone | | Walking, Watching, Charging, Recovering, Stunned | beze změny |
| StartStanding | Placed | | BlowerInit | blower Placed |
| Standing, Hanging, Falling | beze změny | | Blowing | Blowing |
| Splash, Swimming, SwimCalling, SwimWaving, SwimBoarding, Sinking | beze změny | | BlowerWait | Stunned |
| bonus Falling, Lying | beze změny | | TreeInit / Tree / TreeWait / Inactive | tree Placed / Swaying / Resting / Bare |

### Testovací pilot

`CheatPilot.injectHit` (zápis stavu cestujícího do paměti) se nahradí skutečnou akcí: vrtulník s visícím
cestujícím nad letícím flyerem a fire (jako dnešní `dropOnEnemy`, jen s lepším předstihem). Zásahy pak zůstanou jen
tři: přemístění vrtulníku, energie, životy. Pokrytí stavů (`GoldenReplayTest`) musí zůstat úplné.

## 10. Nástroj `replay_check` a C API

**`tools/replay_check`** (C++, linkuje `ugh_logic` + `ugh_logic_replay`, ne C API):

1. tick 0 → `NewGameSettings` z `game.level`, `game.players`, `game.difficulty`, `game.rng`, `game.rainFloor`;
2. každý další tick: `game.step()`, porovnat `StateWriter` s očekávaným stavem (množina polí i hodnoty, včetně
   `game.phase`), pak `I` řádek přes `Cheats` a skenkódy přes `game.scancode()`;
3. při první neshodě vypsat tick, fázi, všechna rozdílná pole (očekávané / skutečné), klávesy posledních snímků
   a skončit; přepínač `--continue` počítá dál (pro statistiku), `--only <skupiny>` jen pro rozjezd v N5–N6.
4. souhrn na řádek: replay, ticků, porovnaných hodnot po skupinách, výsledek. CTest: jeden test na replay.

**C API `include/ugh_logic.h`** (pro UE, krok 10): `ugh_logic_create(data_path, err, size)`, `ugh_logic_destroy`,
`ugh_logic_new_game(settings)`, `ugh_logic_scancode`, `ugh_logic_step` (→ continue / game over / all done),
`ugh_logic_take_events`, a pohled pro vykreslení `ugh_logic_view` (pole entit: druh, poloha, sprite, bublina; vrtulníky;
voda; kapky). Žádné nastavování polí zvenku. Návrh hlavičky v N4, pohled v N8.

## 11. Kroky

### N1 – Průzkum: co ze staré paměti logika opravdu potřebuje

Na současném `sim/` (je napůl sémantické a projde všemi replayi) dočasně přidat měření (`#ifdef UGH_AUDIT`, do
repozitáře necommitovat nebo jen do větve) a spustit všech 161 replayů. Zjistit:

1. **Závisí známý stav na zbytcích paměti?** Přehrávač v režimu celé hry bez převzetí neznámých polí
   (`Unknown::Count` místo `TakeOver`): když známá pole dál souhlasí, zbytky paměti hru neovlivňují a nové jádro je
   může ignorovat. Každé pole, které by se rozešlo, zapsat s příčinou.
2. **Čte originál sdílené slovo v jiném významu**, než v jakém ho naposledy zapsal? Do tříd `SharedWord` přidat
   „poslední význam“ (zápisové metody ho nastaví, čtecí zkontrolují). Výsledek: tabulka (entita, stav, slovo,
   zapsaný význam, čtený význam, počet).
3. **Čtení animace za koncem seznamu:** `Animation::frame` / `endsAt` s indexem za ukončovací značkou – která
   animace, index, stav entity, počet. Rozhoduje, zda UGD 1 potřebuje „přetečení“.
4. **Definovanost polí po stavech:** pro každý druh entity a stav, která pole stav čte dřív, než je přepíše
   (instrumentace čtecích metod entit). Výsledek = podklad tabulky polí UGR 1 (kap. 9).
5. **Ověřit seznam zvláštností** (kap. 6): kde se v replayích skutečně projeví (např. počet přetečení `Int16` na
   operaci, průchody sondy zdí doleva, kapky přes okraj, `lives` pod nulou), a doplnit, co chybí.

Výstup: `re/notes/rewrite-audit.md` (tabulky a rozhodnutí u každého nálezu: pojmenované chování / zahodit / data),
úprava kap. 6 a 9 tohoto dokumentu. **Hotovo když** každý nález má rozhodnutí a `sim/` je beze změny (měření jen
lokálně nebo ve větvi).

### N2 – Data UGD 1

- `extractor`: `Names.kt` (jména druhů, bonusů, animací), `LogicData.kt` (zápis UGD 1; aritmetika dnešního
  `GameDataLoader.cpp` převedená do Kotlinu: Amiga → PC, polohy z plošin, rychlost walkera `(s >> 2) − s`, y foukače,
  zpoždění tras × 3/2, strop vody), volání z `Main.kt` vedle `ugh-sim.bin`.
- Test v `verify` (`LogicDataTest`): pro každý level obou režimů po načtení levelu v Kotlin portu porovnat hodnoty
  z paměti (plošiny, start vrtulníků, voda, vítr, umístění a rychlosti nepřátel, poloha stojícího cestujícího, trasy
  a bonusy stromů) s UGD 1; masky s dnešním `MASK`.
- Popis formátu do `phase2-data.md` (nová sekce), aktualizovat kap. 8 tady.
- **Hotovo když** `.\gradlew.bat :extractor:run` vytvoří `assets/sim/ugh-data.ugd`, test je zelený pro všech 81
  levelů a soubor jde přečíst okem (ukázka levelu 1 v `phase2-data.md`).

### N3 – Replaye UGR 1

- `verify/.../replay/SemanticProjection.kt`: paměť originálu → pole UGR 1 podle kap. 9 (jména z `Names.kt`, významy
  sdílených slov podle stavu, pravidlo úplnosti jako tabulka po stavech podle výsledku N1). `ReplayWriter` umí
  `UGR 1` (bez `w=` a `B`); `GoldenReplayTest` píše z jednoho běhu oba formáty (`replays/*.ugr` = UGR 0 pro `sim/`,
  `replays/ugr1/*.ugr`), port a originál dál porovnává po snímcích.
- `CheatPilot`: `injectHit` nahradit akcí (kap. 9); zásahy jen vrtulník, energie, životy. Pokrytí stavů úplné
  (případně vylepšit předstih `dropOnEnemy`).
- Kontrola v testu: každé pole UGR 1 je v tabulce pravidel, žádný stav nemá `?`, žádná hodnota není adresa.
- Finální tabulka polí do kap. 9.
- **Hotovo když** `.\gradlew.bat :verify:replays` vytvoří 161 replayů v obou formátech, `sim/` dál projde UGR 0
  (CTest zelený) a `:verify:test` je zelený.

### N4 – Kostra `logic/`

- CMake (knihovny `ugh_logic`, `ugh_logic_replay`, nástroj `replay_check`, testy), `build.ps1`, `README.md` (kostra),
  `re/notes/logic-map.md` (prázdná tabulka).
- `units/` + testy; `data/` (definice, `DataFileReader` pro UGD 1 s kontrolami, `GameData`) + testy (načte reálný
  soubor, chybové hlášky pro poškozený vstup); `events/`.
- `replay_check`: parser UGR 1, porovnání, hlášení, `--only`; `game::Game` zatím jen nová hra a fáze před popiskem.
- Návrh hlavičky `include/ugh_logic.h` (bez pohledu).
- **Hotovo když** build a testy zelené a `replay_check` u všech replayů souhlasí v `game.*` až do první fáze
  `caption`.

### N5 – Tok hry, načtení levelu, vrtulník

- `world/` (Session, Level, Copter, Pad, Water, Rain, Energy, Fade, Animator, Screen, RandomNumbers), `physics/`
  (CopterPhysics, CollisionProbe, TouchBox), `input/PcKeyboard`, `game/` (GameFlow s fázemi, PlayFrame,
  LevelLoader, Cheats).
- Testy jednotek: fyzika (přistání, náraz, voda, vítr), sonda (obě zvláštnosti), voda, déšť, klávesnice, fáze.
- **Hotovo když** v replayích levelu 1 souhlasí `game.*` (bez skóre a cestujících), `copter.*` (bez nákladu) a `pad.*`
  (bez `waiting`) až do prvního nástupu cestujícího; u ostatních replayů je první neshoda způsobená chybějícím
  modulem (ručně ověřit aspoň 5 replayů různých levelů).

### N6 – Cestující a bonusy

- `passengers/` (oba automaty), `physics/Ballistics`, `bonuses/` (BonusItem, BonusSlots, Falling, Lying).
- Testy scénářů: cestující s trasou (nástup, jízda, platba, bonus za rychlé doručení, voda po kolena, sražení do
  vody, plavání, záchrana, utonutí), stojící (vyzvednutí, puštění, dopad na plošinu), bonus (pád, ležení, sebrání).
- **Hotovo když** replaye levelu 1 souhlasí ve všem kromě `enemy.*`, dokud hra nepřejde do levelu 2.

### N7 – Nepřátelé

- `enemies/` (Flyer, Walker, Blower, Tree, jejich stavy, EnemyFactory), odraz padajícího cestujícího, shazování
  bonusů ze stromu.
- Testy scénářů pro každý druh (flyer loví cíl a srazí vrtulník, zásah cestujícím; walker sleduje, nabíhá, odhodí
  vrtulník, omráčení; foukač tlačí; strom shodí bonus a odpočívá).
- `--only` z nástroje odstranit.
- **Hotovo když všech 161 replayů projde celých**: každé pole, každý snímek, stejná množina polí.

### N8 – Dokončení

- `README.md` (průvodce, tabulka „chci změnit X → soubor Y“, slovníček), `logic-map.md` vyplněný, pohled pro
  vykreslení v C API, testy C API.
- `/code-review` na `high`, opravit vše o čitelnosti, struktuře a DRY; junior test (kap. 12).
- Smazat `sim/`, zápis UGR 0, `StateProjection` jen jako podklad sémantické projekce (nebo sloučit), řádky `B`
  (`StageRecorder`), `ugh-sim.bin` (`Sim.kt`), pokud ho nic jiného nepotřebuje. Plán kroků 10–11 přepnout na `logic/`.
- **Hotovo když** platí kap. 12.

## 12. Hotovo když (celé nové jádro)

- Všech 161 replayů UGR 1 projde celých (stejná pole, stejné hodnoty, každý snímek); testy jednotek a scénářů po
  modulech zelené; `build.ps1` zelený.
- V `logic/src` není (kontrola grepem): `DGROUP`, `113b`, `0x` mimo bitové masky, `static_cast<int16_t>`, `throw`,
  `try`, `dynamic_cast`, `typeid`, vlastní `template`, `#define`, `std::function`, `co_await`, veřejné měnitelné pole
  entity, `-1` jako „nic“.
- Každý soubor jedna třída se jménem souboru; složka = namespace všude; závislosti jen jedním směrem (kap. 3.9).
- Junior test podle `README.md`: u požadavků (1) změnit gravitaci vrtulníku, (2) prodloužit, jak dlouho cestující
  volá, (3) přidat událost pro frontend, (4) přidat pole do replaye, (5) přidat nový druh nepřítele, (6) změnit
  počet životů na začátku jde ukázat jeden až tři soubory, kam sáhnout.
- `/code-review` na `high` bez nálezů na čitelnost, strukturu a DRY.

## Jak dopadly kroky

(sem se po každém kroku zapíšou odchylky od návrhu)

### N1 (2026-10-03)

Výsledky a rozhodnutí v [rewrite-audit.md](rewrite-audit.md); měření ve větvi `audit/n1`, `sim/` na `main` beze změny.
Odchylky od návrhu: `copter.impact` a `water.row` nejsou stav (vypadly z kap. 9), `copter.effort` je stav i mezi
pokusy, poloha cestujícího „viděná minulý snímek“ je jen u cestujících s trasou a jmenuje se `seenX/Y`, nepřítel
v Placed nemá sprite, maska v UGD 1 jen 320 px (sousední řádek nikdy nenarazí), animace bez přetečení, nové
zvláštnosti v kap. 6.
