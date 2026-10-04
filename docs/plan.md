# Plán: UGH! remake v UE 5.8 - krok za krokem (1 krok = 1 session)

Kontext a výsledky zkušebního průchodu: [phase4-modernization.md](../2_reverse_engineering/notes/phase4-modernization.md).
Pravidla platná pro všechny kroky:

- Fyzika a logika **přesně jako originál**. Kotlin port (`core/`) je jen reference a generátor testů, nepřepisuje se.
- Testy jsou data: golden replays (`verify/src/test/kotlin/ugh/verify/replay/`), každá nová implementace musí projít
  všemi replayi pole po poli. Od kroku N8 jen sémantické `UGR 1` (pojmy hry, žádné adresy; `UGR 0` zaniklo se
  starým jádrem `sim/`), podle nich se ověřuje jádro `logic/`.
- Herní data se necommitují (jen kód); C++ jádro čte data vytažená extractorem do `assets/`.
- Nic viditelného na notebooku bez souhlasu (okno hry, editor se scénou, Blender); příkazy pro Jana jen PowerShell 5.1.
- Na konci každého kroku: testy zelené, krátký zápis do tohoto souboru (sekce Stav), commit po Janově souhlasu
  (kroky 13+ v noci 2026-10-04: commit a push rovnou, schváleno).

## Krok 1 - Úklid a commit dosavadní práce

- Projít necommitované změny (`verify/.../replay/*`, `Lockstep.kt` posluchač kláves, `re/notes/phase4-*.md`,
  `re/notes/plan.md`); smazat nepoužitý `setup/install-dev.ps1`.
- Golden replays generovat Gradle úlohou (např. `:verify:replays`) do `verify/build/replays`, ne jen jako vedlejší
  produkt testu.
- Hotovo když: `.\gradlew.bat :verify:test` zelený, změny commitnuté.

## Krok 2 - Replays napříč všemi levely

- Problém: náhodný i cheat pilot zůstávají v levelu 1.
- Start z libovolného levelu přes heslo (F2 v menu, hesla v DGROUP od `00ee` / `0492`) - stále jen vstupy z klávesnice.
- Sada: pro každý level (69 jeden hráč, 81 team) krátký cheat záznam (~2000 snímků) + několik dlouhých
  náhodných záznamů na různých obtížnostech; dokončení levelu (doručit všechny) aspoň v několika levelech.
- Hotovo když: každý level má replay, ve všech 0 rozdílů, pokryté všechny pojmenované stavy cestujících,
  objektů i bonusů (test to vyhodnotí a vypíše chybějící).

## Krok 3 - Audit úplnosti stavu

- Instrumentovat zápisy do paměti portu během hraní a vypsat proměnné DGROUP/CS, které se mění, ale projekce je
  nezahrnuje (animace vrtulníku, stavový řádek, déšť, časovače, ...).
- U každé rozhodnout: patří do stavu (ovlivňuje logiku nebo ji potřebuje renderer), nebo je čistě vnitřní/VGA.
- Doplnit `StateProjection`, přegenerovat replays.
- Hotovo když: seznam neprojektovaných proměnných je prázdný nebo každá má zdůvodnění v kódu.

## Krok 4 - Export dat pro C++ jádro

- Extractor vyexportuje z UGH.EXE do `assets/sim/` vše, co logika potřebuje: mapy levelů, záznamy levelů
  (26 B), seznamy A-D, deskriptory cestujících/objektů/bonusů, tabulky animací, hesla, a hlavně
  **pixelovou kolizní masku** (kolize jsou po pixelech, bit 7 barvy pozadí, `113b:1457`).
- Formát jednoduchý pro C++ (binární s hlavičkou nebo text), popsat v `phase2-data.md`.
- Hotovo když: test v `verify` ověří, že kolizní maska z exportu = pozadí ve VGA portu pro všechny levely.

## Krok 5 - C++ jádro: kostra a vrtulník

- `sim/` v repu: C++20, CMake (Build Tools 2026), bez závislostí; C API (`ugh_sim_*`).
- Přehrávač replayů (`UGR 0`, včetně `I` řádků) jako konzolový program + CTest.
- Implementovat: načtení levelu, start pozice, fyzika vrtulníku (`113b:1095`), přistání, náraz/crash, energie, RNG.
- Porovnávat jen pole `game.*` a `copter.*` (filtr v přehrávači), zbytek zatím přeskočit.
- Každá funkce s odkazem na adresu originálu a funkci Kotlin portu.
- Hotovo když: všechny replays projdou pro `copter.*` a odpovídající `game.*`.

## Krok 6 - C++ jádro: cestující a plošiny

- Stavový automat cestujících (`113b:1486..2276`), plošiny, nástup, jízda, platba, bonusy za rychlé doručení.
- Hotovo když: projdou `passenger.*` a `pad.*`.

## Krok 7 - C++ jádro: nepřátelé, stromy, foukače, bonusové předměty

- `113b:2363..2b7e` a `2b7f..2d1b`.
- Hotovo když: projdou `object.*` a `bonus.*`.

## Krok 8 - C++ jádro: průběh hry

- Fáze (`setup`, `caption`, `play`, `betweenLevels`, ...), fade, konec levelu, ztráta života, game over, skóre,
  multiplikátor, team mode.
- Hotovo když: **všechny replays projdou celé**. Volitelně FFM most do `verify` pro lockstep ladění C++ vs Kotlin.

## Krok 9 - C++ jádro: přepis do čisté architektury

Současné jádro je věrný přepis assembleru (paměť DGROUP na adresách, registry `Regs`, skoky přes adresy
obslužných rutin). Je přesné, ale nečitelné. Cíl: kód, který je radost číst. Logika zůstává bit po bitu stejná,
replays jsou záchranná síť.

- **Typovaný model místo paměti:** `World` (level, hráči, voda, déšť, skóre), `Copter`, `Pad`, `Passenger`,
  `Enemy`, `BonusItem`. Žádné offsety DGROUP v logice; zůstanou jen ve dvou okrajových vrstvách (načtení dat
  a projekce pro replays).
- **Data levelu a deskriptory jako typy:** `LevelDefinition`, `PassengerKind`, `EnemyKind`, `BonusKind`,
  `Animation` (snímky + prodleva), načtené z `ugh-sim.bin` továrnou (Factory). Logika nečte syrová slova.
- **Stavové automaty jako vzor State:** stav cestujícího / nepřítele / bonusu je objekt s jasnými přechody,
  ne adresa rutiny a `jumpVia(r, 0x24)`. Sloty deskriptoru (+08 … +2c) se stanou pojmenovanými přechody
  (`onLanded`, `onHit`, `onDone` …).
- **Druhy nepřátel jako Strategy:** pterodaktyl, walker, foukač a strom sdílí rozhraní `EnemyBehavior`.
- **Hodnotové typy pro aritmetiku originálu:** `Fixed` (1/32 px, 16bit wrap), `Velocity`, `Int16` se stejným
  přetečením a posuny, aby se v kódu nemuselo všude psát `w16(s16(...))`.
- **Oddělené služby:** `CollisionMask` (sonda `1457`), `Random` (`4f09`), `Input` (klávesová tabulka →
  `Controls` hráče), `Rain`, `Water`.
- **Události místo vedlejších efektů (Observer):** zvuky, sebrání bonusu, doručení, havárie jako události pro
  frontend (UE si podle nich přehraje zvuk nebo efekt), ne zápisy do proměnných stavového řádku.
- **Pryč s balastem:** nic z vykreslování VGA (pozice „kreslil jsem minule“, stavový řádek, scratch
  proměnné, `prepare()`). Zvláštnosti originálu (třeba neposouvaná sonda při pohybu doleva) pojmenované
  a zdokumentované v kódu, ne schované v přepisu instrukcí.
- **Adaptér pro replays:** `ReplayProjection` převádí model na pole replayů a zpět. Je to jediné místo se jmény
  a formáty `UGR 0`.
- Testy jednotek pro služby a automaty vedle replayů. Rozhraní C API zůstane (UE ho používá od kroku 10).
- Hotovo když: všech 161 replayů projde celých, v logice nejsou offsety DGROUP ani `Regs` a kód projde code
  review (`/code-review`) bez nálezů na čitelnost.

## Krok 9b - C++ jádro: čitelná struktura (model, stavy, data)

Podle [core-design.md](../2_reverse_engineering/notes/core-design.md), etapy 1-5: hodnotové typy, složky a namespaces, rozdělená data, entity jako
třídy s Mementem, stavy jako třídy (State) s `enter/update`. Hotovo když: 323 testů zelených po každé etapě.

## Krok 9c - C++ jádro: fyzika, tok hry, adaptér, průvodce

Podle [core-design.md](../2_reverse_engineering/notes/core-design.md), etapy 6-11: zbytek `physics/`, tok hry jako automat fází místo korutin,
adaptér replayů jako Visitor, rozdělený přehrávač a testy, `sim/README.md`. Hotovo když: splněna sekce
„Hotovo když“ v core-design.md.

## Nové jádro `logic/` (kroky N1–N8)

Review 2026-10-03: `sim/` je přesné, ale model pořád kopíruje paměť originálu (sdílená slova, jedna třída pro čtyři
nepřátele, sloty se zbytky, DGROUP za běhu, jména podle adres). Rozhodnutí: **nová, čistě napsaná C++ aplikace
`logic/`**, totožná s originálem 1:1, bez čehokoli z paměti staré aplikace; všechno o paměti DOSu jen v Kotlinu.
Hranice jsou data `UGD 1` a sémantické replaye `UGR 1`. Zadání všech kroků, architektura, formáty a „Hotovo když“:
[rewrite-design.md](rewrite-design.md). `sim/` se do N8 nemění (reference, UGR 0 dál prochází).

- **N1 – Průzkum** (kap. 11 N1): na `sim/` dočasně změřit, zda známý stav závisí na zbytcích paměti, zda se sdílená
  slova čtou v jiném významu, čtení animací za koncem, definovanost polí po stavech, zvláštnosti. Výstup
  `rewrite-audit.md`. Hotovo když má každý nález rozhodnutí a `sim/` je beze změny.
- **N2 – Data UGD 1**: `extractor` (`Names.kt`, `LogicData.kt`) → `assets/sim/ugh-data.ugd`, test proti paměti portu
  pro všech 81 levelů, popis v `phase2-data.md`.
- **N3 – Replaye UGR 1**: `SemanticProjection.kt`, `ReplayWriter` pro UGR 1, oba formáty z jednoho běhu,
  `CheatPilot` bez zápisu stavu cestujících. Hotovo když 161 replayů v obou formátech a `sim/` dál zelené.
- **N4 – Kostra `logic/`**: CMake, `units/`, `data/` (parser UGD 1), `events/`, nástroj `replay_check`, návrh C API.
  Hotovo když `game.*` souhlasí až do prvního popisku.
- **N5 – Tok hry, načtení levelu, vrtulník**: `world/`, `physics/`, `input/`, `game/`. Hotovo když v levelu 1
  souhlasí hra, vrtulníky a plošiny do prvního nástupu.
- **N6 – Cestující a bonusy**: oba automaty cestujících, `Ballistics`, `bonuses/`. Hotovo když level 1 souhlasí ve
  všem kromě nepřátel.
- **N7 – Nepřátelé**: flyer, walker, foukač, strom. Hotovo když **všech 161 replayů projde celých**.
- **N8 – Dokončení**: README, `logic-map.md`, pohled pro vykreslení v C API, `/code-review high`, junior test,
  smazat `sim/`, UGR 0 a `ugh-sim.bin`.

## Krok N9 - Repo po krocích, C++ jádro bez kompromisů

Review 2026-10-03 po N8: `logic/` je čitelné, ale ne vzorové (zdvojený stavový automat, tlusté třídy, PC scancody
a 16bitová aritmetika DOSu v API a modelu, 565řádkový `DataFileReader`). **Kotlin se nepřepisuje ani neuklízí** -
je to jen reference a generátor testů. Upravuje se jen C++ a umístění složek. Po každé etapě testy jednotek
a všech 161 replayů zelené (každé pole, každý snímek).

### N9a - Složky podle kroků projektu (obsah Kotlinu beze změny)

Číslo = krok řetězu; odkazy vedou jen dozadu (pozdější krok používá dřívější), uvnitř kroku mezi sousedy.

```
1_original/             UGH.EXE (dnes OLD/, necommituje se)
2_reverse_engineering/  dnešní re/: Ghidra skripty, JS nástroje, poznámky k originálu
3_kotlin_port/          core/, oracle/ (emulátor originálu), desktop/ (hratelný port)
4_test_data/            extractor/ (-> assets/logic/ugh-data.ugd), verify/ (-> golden replaye UGR 1)
5_remake/
  logic/                C++ pravidla hry: knihovna + testy jednotek (o replayích neví)
  game/                 UE projekt (krok 10), logiku přibalí jako modul z ../logic
6_verification/         přehraje replaye ze 4 proti logice z 5, pole po poli
docs/                   plán, návrh a mapa C++ jádra
assets/                 vygenerovaná data (necommitují se)
```

- `git mv` podle stromu. Gradle wrapper, `settings.gradle.kts` a `build.gradle.kts` zůstanou v kořeni, takže
  `.\gradlew.bat :extractor:run` a `:verify:replays` se nemění.
- `docs/`: z `re/notes/` vytáhnout `plan.md`, `rewrite-design.md`, `logic-map.md`; zbytek (`phase*`, `core-design.md`,
  `rewrite-audit.md`) zůstane v `2_reverse_engineering/notes/`.
- `6_verification/`: sem z `logic/` přejde `replay/` (zápis stavu do polí UGR 1) a `tools/replay_check/`; vlastní
  `CMakeLists.txt` (logiku přidá `add_subdirectory(../5_remake/logic)`), `build.ps1` (logika + testy jednotek +
  161 replayů) a `README.md`. `5_remake/logic/build.ps1` staví jen knihovnu a testy jednotek.
- Úpravy jen mimo Kotlin kód: `settings.gradle.kts` (`project(":core").projectDir = file("3_kotlin_port/core")` ...),
  cesty `OLD/` -> `1_original/` v `build.gradle.kts` extractoru, oracle a verify, cesta k `package/README.txt`
  v `build.gradle.kts` desktopu, `.gitignore`, `release.yml`, odkazy v C++ a v `docs/`. Odkazy na `re/notes`
  v komentářích Kotlinu zůstanou (Kotlin se nemění).
- `README.md` v kořeni: co je který krok a v jakém pořadí číst; krátké `README.md` v každé číslované složce.
- Hotovo když: `.\gradlew.bat :extractor:run :verify:replays` projde, `6_verification\build.ps1` zelený a `git status`
  ukazuje u Kotlinu jen přejmenování (a výše uvedené `.kts`).

### N9b - C++ jádro (etapy, po každé zelené)

1. **Jeden stavový automat (DRY):** šablona `state/StateMachine<Entity, Context>` + `state/State<Entity, Context>`
   (`enter`, `update`, `name`) místo 7 kopií `changeState` / `continueIn` / `state_` a 7 skoro stejných rozhraní
   `*State`. Pravidlo „žádné vlastní šablony“ padá (UE šablony nevadí); dál platí bez výjimek, RTTI a maker.
2. **Zapouzdření entit:** `Passenger` a `Enemy` bez `protected` dat. `RoutePassenger` (~35 veřejných metod) rozdělit
   na malé části s vlastním chováním - `PassengerCall` (volání / mávání), `Swim` (rychlost, čas na hladině),
   `Ride` (nosič, čas rychlého doručení), `Route` (zastávky, zpoždění) - stavy volají záměry
   (`passenger.ride().start(copter)`), ne settery. Totéž u `Walker` a `Flyer` (časovače).
3. **Druhy cestujících po jednom:** `PassengerKind` s `Type` a poli platnými jen pro část typů nahradit třídami
   `RoutePassengerKind`, `SwimmerKind`, `StandingPassengerKind`; přepnutí do vody a zpět jako dvojice druhů. V `data/`
   jedno pravidlo pro všechny záznamy: neměnné struktury s veřejnými poli; umístění (Visitor) stejně, jen s `accept`.
4. **Vstup bez DOSu:** C API `ugh_logic_key(player, key, pressed)` a `ugh_logic_menu_key(key)` (Esc, P, „jiná klávesa“
   pro popisek) místo scancodů. `PcKeyboard` (scancody, rozšířené klávesy, falešné shifty) se přesune do `6_verification/` jako
   adaptér, který z kláves replayů dělá tyto vstupy. Logika o PC klávesnici neví.
5. **Typy hry místo registrů:** podle `rewrite-audit.md` 16bitové přetečení ani neznaménkové porovnání v replayích
   nenastane, takže jízdné, vzhled, číslo plošiny, životy, skóre a časovače budou `int`, `unsignedLess` zmizí
   a 16bitová sémantika zůstane jen uvnitř `Fixed` / `Speed`. Nutné zvláštnosti originálu (sonda doleva / nahoru,
   pixelová poloha vrtulníku po hodu, kapky přes okraj stránky) jen uvnitř své třídy, pojmenované a popsané. C API
   dává kapky v souřadnicích obrazovky (bez „stránky 384 px“).
6. **Zbytek DRY a čitelnost:**
   - prodleva animace walkera jednou (dnes 4×),
   - `RoutePassenger::stepTowards(x)` místo 3 kopií chůze,
   - stavy na plošině se společnou kontrolou pádu do vody (dnes ve 4 stavech),
   - zóna foukače přes stejnou třídu jako `TouchBox`,
   - `copterOnWater(true, false)` -> pojmenované dotazy (`copterOnWaterWithRoom()` ...),
   - metody `Game` pro fáze (`startAttempt`, `endAttempt` ...) do vlastní třídy, kterou dostane jen `GameFlow`,
     takže na fasádě zůstane jen veřejné API,
   - smazat mrtvou `PcKeyboard::LONGEST_SEQUENCE`.
7. **Čtení dat:** `DataFileReader` (565 ř.) rozdělit na `UgdTokenizer` (řádky -> záznamy), `RecordReader` (klíče,
   čísla, chyby s číslem řádku) a čtenáře po částech (`KindsReader`, `LevelReader`, `KeysReader`); `DataFileReader`
   je jen skládá.
8. **Dokumentace a kontrola:** `5_remake/logic/README.md`, `6_verification/README.md` (vzory, „kam sáhnout“), `docs/rewrite-design.md` (pravidla),
   `docs/logic-map.md`; `/code-review high` bez nálezů; junior test (3 vymyšlené požadavky, u každého jeden soubor).
   Vlastní testovací framework (68 ř., bez závislostí) zůstává.

Hotovo když: po každé etapě 161 replayů a testy zelené. Grep v `5_remake/logic/src`: žádné `scancode`, `unsignedLess`,
`changeState` mimo `state/`, `protected:` s daty. Žádná třída nad ~15 veřejnými metodami (kromě fasády `Game`)
a žádný soubor nad 200 řádků.

### N9c - C++ jádro: objekty místo indexů, balíčky, nic navíc pro testy

Review 2026-10-03 po N9b: jádro je čisté (žádné adresy, registry ani scancody), ale zbylo myšlení originálu (entity
přes čísla v polích), `friend` přístupy do cizích vnitřků, háčky testovacího pilota v doménovém modelu, plochá
složka `data/` (47 souborů, model i čtení souboru) a nekonzistentní zapouzdření nepřátel. Etapy (po každé zelené):

1. **Objekty místo indexů:** logika pracuje s `world::Copter&` / `Copter*` a `world::Pad&`, ne s čísly hráčů a plošin
   (`Copters::landedOn`, `TouchBox::firstCopterIn`, nosič cestujícího, cíl flyeru). Vrtulník zná své `player()`,
   plošina své `index()` - čísla jen pro eventy, C API a replaye. Kontexty bez mezikroku: `PassengerContext`
   a `EnemyContext` jsou `PlayContext` + své navíc (`context.level`, ne `context.play.level`); cestující má dotazy
   `pickupPad(level)` / `targetPad(level)` místo řetězů `level.pad(passenger.route().pickupPad()).place()`.
2. **Bez `friend` do cizích vnitřků:** stav hry (session, level, cestující, nepřátelé, bonusy, menu) je
   `game::GameState`; `Game` (fasáda) i `Attempts` s ním pracují explicitně. Čtenáři dat plní `GameData::Contents`,
   ze kterého vznikne neměnné `GameData` (žádný `friend`).
3. **Testovací pilot pryč z logiky:** `Cheats` a metody `*ByTestPilot` se přesunou do `6_verification` jako
   `TestPilot`; v logice zůstane jen deklarace `friend class ugh::testing::TestPilot;` u tříd, které pilot mění
   (vzor test peer, žádný kód). `State::name()` zůstává (je to jméno stavu, ne háček) a stavy `Placed` taky
   (je to stav entity před první aktualizací, replaye ho vidí během popisku).
4. **Balíčky:** `data/` rozdělit na `data/` (`GameData`, `Rules`, `SpriteIds`, `Animation`, `Box`, bonusy),
   `data/kinds/` (druhy cestujících a nepřátel), `data/level/` (definice levelu, plošiny, maska, vítr, trasy,
   umístění a jejich Visitory) a `data/ugd/` (čtení souboru UGD 1: tokenizer, `RecordReader`, čtenáři,
   `DataFileReader`, builder). `PlayerKey` do `input/`.
5. **Čtení dat DRY:** jedna tabulka typů záznamů místo tří seznamů (`KNOWN`, `isEnemyKind`, `isLevelPart`);
   `readLevelPart` rozdělit na metodu po typu záznamu; `RecordReader` čte aktuální záznam (bez opakování `r`
   v každém volání); plošina po jménech polí, ne `int p[7]`.
6. **Zapouzdření nepřátel:** žádné `Countdown&` ven; záměry (`stun()`, `stunOver()`, `hideFor()`, `restOver()` ...),
   omráčení walkera a foukače jedna část `enemies::Stun` (jedna `STUN_TIME`).
7. **API a jednotky:** `ugh_logic_default_settings()` (frontend nemusí znát řádek deště 180 ani tvar semínka);
   převody jednotek pojmenované (`Fixed::half()`, `Speed` na `Fixed` ...) místo holých posunů `<< 4`, `>> 5`.
8. **Dokumentace a kontrola:** README logiky a verifikace, `rewrite-design.md`, `logic-map.md`; `/code-review high`
   bez nálezů; junior test.

Hotovo když: po každé etapě 161 replayů a testy zelené. Grep v `5_remake/logic/src`: žádné `friend` kromě
`TestPilot` a továren svých kolekcí, žádné `ByTestPilot`, `Cheats`, `std::optional<int>` pro vrtulník, žádné
`context.play.`; žádný soubor nad 200 řádků.

## Krok 10 - UE projekt v repu, šedé kostky

- `5_remake/game/` (UE 5.8 C++ projekt), C++ jádro `5_remake/logic/` jako UE modul (stejné zdrojáky), pluginy DLSS/FSR jako v UghTrial
  (FSR jen upscaler: `r.FidelityFX.FI.Enabled=0`, `OverrideSwapChainDX12=0`; offscreen oprava FSR).
- Level z mapy dlaždic a kolizní masky jako jednoduché kostky; vrtulník, cestující, nepřátelé jako tvary.
- Pevný tik 70,086 Hz + interpolace pro vykreslení; ovládání klávesnicí (písmena, kvůli české klávesnici).
- Replays jako UE automatické testy (`UnrealEditor-Cmd -nullrhi`).
- Hotovo když: level 1 jde odehrát a replays projdou i uvnitř UE.

## Krok 11 - Vizuální směr „Pravěké dioráma“

- Krátký koncept (paleta, materiály, světlo, kamera), pak první level: útes v řezu generovaný z mapy dlaždic,
  ohniště s Lumen/RT, voda, mlha; herní rovina zůstává přesně podle kolizní masky.
- PSO cache pro balení (bez trhání na startu), skript na balení (`NO_PROXY += ::1`).

## Krok 12 - Celá hra v diorámatu

- Všech 81 levelů (oba režimy): `shot.ps1 -Level <n>` (start levelem přes `first_level`) a `levels.ps1`, který bez
  okna nafotí všechny do přehledu (kontaktní arch PNG); opravit, co v některém levelu nesedí (ohniště, voda, okraje).
- Vítr a déšť: kapky jako plastelínové čárky, mlha a světlo podle větru; stoupající voda.
- Menu před hrou: jeden hráč / tým, obtížnost, heslo levelu (`assets/levels.json`), v týmu dva vrtulníky a druhý
  pilot (W A S D); konec hry zpět do menu.
- Hotovo když: v přehledu žádný level nemá vadu, celou hru jde spustit z menu v obou režimech, replays zelené.

## Kroky 13-23 - Hra plně ve 3D v moderní kvalitě

Zadání (Jan, 2026-10-04): hra plně ve 3D, pěkné modely v moderní kvalitě, postavička vidět v kabině vrtulníku, ohně,
louky, palmy a další dekorace. Herní rovina zůstává pixelově podle kolizní masky, kamera z boku (2,5D), logika beze
změny. Pravidla navíc:

- **Assety jen lokálně, nikdy v gitu:** volně použitelné (CC0 nebo zdarma i pro komerční použití bez přihlášení: Poly
  Haven, ambientCG, Kenney, Quaternius ...) do `assets/3d/` (gitignore), celkem do 100 GB. Stahuje je skript
  `5_remake/game/fetch-assets.ps1` podle manifestu v gitu (`5_remake/game/Assets.json`: zdroj, licence, cesta), takže jdou
  kdykoli stáhnout znovu. Co vznikne v Blenderu, dělají skripty v gitu (`5_remake/game/Blender/*.py`, `blender -b`).
- **Import do UE commandletem** (jako materiály): `build.ps1` naimportuje assety do `Content/Imported` (gitignore).
  Chybí-li asset, hra spadne zpět na dnešní tvar a zaloguje to (repo bez assetů se dál přeloží a testy projdou).
- Po každém kroku: testy logiky, `6_verification` a UE zelené, `levels.ps1` a kontrola archů, zápis do Stavu, commit
  a push (v noci 2026-10-04 předem schváleno Janem).

## Krok 13 - Zvuk a hudba

- Původní zvuky a hudba z `assets/adlx` vyrenderované emulátorem OPL2 Kotlin portu (`Opl2.kt`, `SoundTimeline.kt`) do
  WAV v `assets/sound/` (Gradle úloha); UE je přehrává podle událostí logiky (`ugh_logic_take_events`), hudba v menu
  a ve hře, hlasitost.
- Hotovo když: každá událost se zvukem originálu má zvuk (test: tabulka událost -> soubor, všechny soubory existují),
  hudba hraje, replays zelené.

## Krok 14 - Knihovna assetů a cesta do UE

- `fetch-assets.ps1` + `Assets.json`, Blender skripty, importní commandlet (Interchange: glTF/FBX, textury, PBR
  materiály), `Content/Imported`, fallback na tvary. První várka: skály, textury skal a trávy, HDRI oblohy, jedna palma.
- Hotovo když: `fetch-assets.ps1` + `build.ps1` na čistém stroji vyrobí `Content/Imported`, palma je vidět ve shotu,
  bez assetů se vše přeloží a testy projdou.

## Krok 15 - Útes a jeskyně ve 3D

- Místo vytaženého řezu tvarovaná skála: řez zůstane přesně v rovině hry, ale hrany a plochy dostanou hloubku,
  zaoblení a šum (vyhlazená síť, displacement, Nanite), PBR materiály skály (Poly Haven / ambientCG) míchané podle
  původní kresby (tráva nahoře, kámen, hlína), krápníky, zadní stěna jeskyně v hloubce, svět pokračuje i za okraji
  obrazovky (útes, džungle v dálce, obloha HDRI) místo dřevěné krabice.
- Hotovo když: ve všech levelech je hrana skály v herní rovině přesně na kolizní masce (test: síť v rovině hry vs.
  maska), archy bez vad.

## Krok 16 - Vrtulník

- 3D model pravěkého vrtulníku (dřevo, kůže, kámen; pedály, rotor z kostí / listů), pilot jeskynní muž, který šlape,
  animovaný rotor podle `rotor_sprite`, cestující sedí v kabině viditelně, visící cestující visí na laně. Druhý vrtulník
  týmu jinou barvou.
- Hotovo když: vrtulník má tělo v herní rovině v rozměrech `COPTER_BODY_*`, postava v kabině je ve shotu vidět.

## Krok 17 - Cestující a nepřátelé

- Jeskynní lidé (riggované modely s animacemi chůze, mávání, stání, plavání, pádu) místo válců; podle stavu
  a spritu logiky vybraná animace. Nepřátelé: pterodaktyl (let), dinosaurus walker (chůze, omráčení), foukač, strom
  (padající ovoce = bonus). Bonusové předměty jako 3D předměty.
- Hotovo když: každý druh entity má model a každý stav, který replaye pokrývají, má animaci (tabulka stav -> animace,
  test), archy bez vad.

## Krok 18 - Příroda a dekorace

- Louky (instancovaná tráva a květiny na horních plochách mimo dráhu postav), palmy, kapradiny, keře, kameny, kosti,
  totemy, chýše, ohně na římsách (víc než jeden, s Lumen světlem), liány z krápníků; rozmístění z mapy dlaždic
  a masky deterministicky (stejný level = stejné dekorace), nic nezasahuje do herní roviny před postavami.
- Hotovo když: každý level má dekorace, nic nezakrývá vrtulník, cestující, plošiny ani čísla plošin (test: průnik
  dekorací s herní rovinou), archy bez vad.

## Krok 19 - Voda, déšť, obloha, světlo

- Voda jako moderní vodní plocha (Single Layer Water: vlny, lom, pěna u skály) se stoupající hladinou podle logiky,
  déšť a mlha jako Niagara podle větru, mraky a obloha, nálada levelu (den, soumrak, noc) podle čísla levelu, Lumen.
- Hotovo když: archy bez vad, ve větrných levelech déšť ve směru větru, voda přesně na `water_level`.

## Krok 20 - Efekty událostí

- Niagara: šplouchnutí cestujícího a vrtulníku, výbuch a kouř při havárii, prach při přistání, jiskry bonusu, peníze
  při platbě, křik ptáka (peří), foukač (vítr), padající ovoce.
- Hotovo když: každá událost logiky má efekt (tabulka, test), zvuk i efekt ze stejné události.

## Krok 21 - Menu a HUD

- UMG: titulní obrazovka s 3D scénou za ní, menu, stavový řádek s ikonami (životy, energie jako ukazatel, skóre),
  popisky levelů v pravěkém stylu, konec hry.
- Hotovo když: snímek menu a hry ve shotu, test `Ugh.Menu` dál zelený.

## Krok 22 - Nastavení a ovládání

- Gamepad, přiřazení kláves, nastavení grafiky (upscaler, kvalita, rozlišení), hlasitost, tabulka nejlepších skóre,
  poslední heslo; uložení do `Saved`.
- Hotovo když: test nastavení (uložit / načíst), ovládání gamepadem projde automatický test vstupu.

## Krok 23 - Vydání

- Balení s PSO cache, výkon (cíl 60+ fps na Radeonu 890M v nízkém nastavení, DLSS na RTX), zip s assety vedle hry
  (jen pro vlastní použití), test doma na RTX 5060 Ti (Jan).
- Hotovo když: zabalená hra projde `levels.ps1` z balíčku, fps v logu nad cílem.

## Průběžně

- MCP: zaregistrovat `unreal` (UE 5.8 plugin, `127.0.0.1:8000/mcp`, jen editor; `AllToolsets` ne - rozbije cook)
  a `rider` (jiný port než IDEA 64342), pak restart Claude Code.

## Stav

- 2026-10-02: zkušební průchod hotový (vč. DLSS SR + Frame Generation doma na RTX 5060 Ti: ~150 → ~450 fps),
  první verze projekce stavu a golden replays (5 záznamů, level 1). Další: **krok 1**.
- 2026-10-02: krok 1 hotový - replays generuje `.\gradlew.bat :verify:replays` (~30 s) do `verify/build/replays`
  (celý `:verify:test` je nahraje taky, ~4,5 min, zelený); `setup/install-dev.ps1` smazán. Další: **krok 2**.
- 2026-10-02: krok 2 hotový - 161 replayů (434 tis. snímků, 41 MB, ~2 min paralelně): každý level obou režimů
  přes heslo s cheat pilotem (2000 snímků), 4 dlouhé cheat (30 000) a 7 náhodných do game over na všech
  obtížnostech od různých levelů; meta navíc `level=` a `password=`. Level dokončen 9× v 7 replayích. Pokrytí se
  měří spuštěním obslužných rutin stavů (Start* stavy na hranici snímku vidět nejsou); nedosažitelné
  `passenger.Idle` a `object.Recovering2` jsou zdůvodněné v `GoldenReplayTest.UNREACHABLE`. Cheat pilot umí
  uletět nabíhajícímu walkerovi (`StartRecovering`). Další: **krok 3**.
- 2026-10-02: krok 3 hotový - `StateAudit` na každé hranici snímku porovná paměť portu (obraz programu až po
  konec DGROUP, bez zásobníku) s předchozí a se čtením projekce. Do projekce přibylo: zbývající cestující,
  hladina vody (jemná pozice, pauza, přepínač, animace hladiny), kontrolní součet deště (respawn kapek bere RNG),
  pixelové pozice a animace vrtulníků a cestujících, další pole plošin, cestujících a objektů. Zbytek je
  v `StateProjection.NOT_PROJECTED` s důvodem (co se kde kreslilo minulý snímek, stavový řádek, VGA stránky
  a paleta, klávesnice, data levelu, pomocné proměnné, zvuk); `audit.txt` v `build/replays`. Replays 87 MB.
  Další: **krok 4**.
- 2026-10-02: krok 4 hotový - `assets/sim/ugh-sim.bin` (`UGHSIM01`, 815 kB, popis v `phase2-data.md`): bloky
  DGROUP, mapy, tabulka spritů na původních adresách a kolizní maska 384 × 192 pro každý z 81 levelů.
  `GoldenReplayTest` porovnává masku se stránkou pozadí portu při vstupu do levelu a každý 64. snímek (81/81 shoda)
  a hlídá, že sonda mimo stránku nikdy nenarazí na pevný pixel. Cheat pilot teď drží vrtulník v rozsahu
  fyziky (předtím ho stavěl nad okraj, kde sonda četla kreslicí stránku). Další: **krok 5**.
- 2026-10-02: krok 5 hotový - `sim/` (C++20, CMake + Ninja z Build Tools 2026, C API `ugh_sim.h`, přehrávač
  `ugh_replay` + CTest, `sim/build.ps1`). Nová hra, start levelu (bez cestujících a objektů), konec levelu
  a snímek hry s fyzikou vrtulníku, vodou, deštěm, klávesnicí a RNG: všech 161 replayů projde (`game.*`,
  `copter.*`, `pad.*`), 7 s. Přehrávač kontroluje každý přechod zvlášť ze zaznamenaného stavu. Aby šlo jádro
  ověřovat po fázích, replay má nové řádky `B` (co cestující / objekty / bonusy změnily mimo svou skupinu).
  Do projekce přibylo `game.rainFloor` (skrytý vstup deště při načtení levelu); `game.rain` je `none` bez větru.
  Další: **krok 6**.
- 2026-10-02: krok 6 hotový - jádro přestavěné na stav jako paměť DGROUP na původních adresách (pole replayů
  jsou pohled na ni, skryté odvozené proměnné doplní `prepare()`), logika převedená z Kotlinu řádek po řádku.
  Cestující (všech 37 rutin `113b:1486..2276`), jejich seznam při načtení levelu, kreslené pozice, platba
  a bonus za rychlé doručení (`bonusSpawn`). Načtení levelu už plní i objekty (seznam C). Všech 161 replayů
  projde i pro `passenger.*` a `pad.*` (např. 1,7 mil. porovnaných hodnot cestujících v `1p-L01-cheat-long`);
  přehrávač vrací jen fáze, které jádro nemá (`ugh_sim_has_stage`). Další: **krok 7**.
- 2026-10-02: krok 7 hotový - objekty (`objects.cpp`: pterodaktyl, walker, foukač, strom, 27 rutin
  `113b:2363..2b7e`, testy blízkosti `2196`, `22f1`) a bonusové předměty (`bonuses.cpp`: `2b7f..2d1b`, `2207`).
  Snímek hry je v jádře celý; všech 161 replayů projde pro všechna pole včetně `game.rng` a `game.rain`
  (celkem 7,9 mil. porovnaných hodnot objektů, 125 tis. bonusů, 21,7 mil. cestujících). Zvuky jsou vynechané
  (handle smyčky mávání `2d61`, který replay nemá, je v jádře 0). Další: **krok 8**.
- 2026-10-02: krok 8 hotový - průběh hry jako C++20 korutiny (`flow.hpp`, `co_await vsync()` na místech, kde
  originál čeká na paprsek): nová hra, černá paleta, popisek (fade in, čekání na klávesu, fade out), nastavení
  levelu, smyčka hry s fade, konec levelu, ztráta života, game over; C API `ugh_sim_step` = jeden snímek.
  **Všech 161 replayů projde celých** od tiku 0 jen z kláves a injekcí: 434 tis. snímků, 0 rozdílů, 82 mil.
  porovnaných hodnot. Převzato 8 808 hodnot, které jádro nikdy nezapsalo (paměť attract módu: cestující
  a `copter.effort/impact/fareMin` na první popisce, pole objektů, která load u daného druhu nepíše).
  CTest: plný běh + `--each` (každý přechod zvlášť), 322 testů, 15 s. FFM most nebyl potřeba. Další: **krok 9**.
- 2026-10-02: krok 9 hotový - jádro přepsané nad typovaný model (`world.hpp`: `World`, `Copter`, `Pad`, `Passenger`,
  `Enemy`, `BonusItem`, `Water`, `Rain`, `Fade`) a datové typy z továrny (`data.cpp`: `LevelDefinition`,
  `PassengerKind`, `EnemyKind`, `BonusKind`, `Animation`, `Route`, `CollisionMask`, tabulka kláves; kontroluje i sloty
  stavů v deskriptorech). Offsety DGROUP zůstaly jen v továrně a v `replay_projection.cpp`, `Regs` ani `jumpVia`
  nejsou. Stavy jako objekty (`PassengerState`, `EnemyState`, `BonusState`) s pojmenovanými přechody, druhy nepřátel
  jako `EnemyBehavior`, `Fixed` (1/32 px, 16bit wrap), služby `Random`, `Keyboard`, `CollisionMask`, voda a déšť,
  události pro frontend (`ugh_sim_take_events`: zvuky originálu, havárie, doručení, bonusy). Zvláštnosti pojmenované
  (sonda doleva / nahoru jen o pixel, sdílená slova slotů, 77fe, 7926). Neznámou paměť (attract mód) pozná přehrávač
  tak, že pouští dvě jádra s různou výplní (`ugh_sim_reset/clear(sim, fill)`) a věří jen shodným polím - jádro samo
  žádné příznaky „známé“ nemá. Všech 161 replayů projde celých i po přechodech se stejným počtem porovnaných hodnot
  jako dřív, 12 testů jednotek (`tests/unit_tests.cpp`), CTest 323 testů, ~28 s. `/code-review`: 6 nálezů opraveno
  (mj. `ugh_sim_create` teď dává výchozí stav programu), zdvojený čas testů ponechán. Další: **krok 10**.
- 2026-10-03: analýza čitelnosti jádra hotová - cílový návrh v [core-design.md](../2_reverse_engineering/notes/core-design.md) (složky a namespaces,
  jedna třída na soubor, entity s Mementem, stavy jako třídy, automat fází místo korutin, adaptér jako Visitor,
  průvodce `sim/README.md`). Další: **krok 9b**.
- 2026-10-03: krok 9b hotový - jádro ve složkách a namespaces (`core/`, `data/`, `model/`, `physics/`, `passengers/`,
  `enemies/`, `bonuses/`, `input/`, `game/`, `replay/`, `api/`, 182 souborů): hodnotové typy `Word` / `Fixed` /
  `Speed` / `Countdown`, továrna `GameDataLoader` oddělená od `GameData` (kontroluje i plošiny tras), entity jako třídy
  se `Snapshot` (Memento) a sdílenými slovy jako malými třídami, `GameSession` + `Level` s dotazy, 41 stavů jako
  třídy s `enter/update`, `EnemyBehavior` jako Strategy, `CopterPhysics` s pojmenovanými kroky. V logice není
  `static_cast<int16_t>`, šablona ani `std::function`. 323 testů zelených, výstup přehrávače shodný s výchozím
  (stejné počty porovnaných hodnot). Odchylky od návrhu v [core-design.md](../2_reverse_engineering/notes/core-design.md). Další: **krok 9c**.
- 2026-10-03: krok 9c hotový - `physics/CollisionProbe` + `TouchBox`, tok hry jako automat fází (`GameFlow`,
  `phases/`, `PlayFrame`; korutiny pryč), adaptér replayů jako Visitor (jeden seznam polí na entitu, bez šablon
  a `std::function`), přehrávač rozdělený (`tools/ugh_replay/`), testy po modulech (`TestFramework.hpp`, 25 testů),
  průvodce `sim/README.md`. 323 testů zelených, výstup přehrávače shodný s výchozím. `/code-review high`: 8 nálezů,
  7 opraveno, 1 ponechán (viz [core-design.md](../2_reverse_engineering/notes/core-design.md)). Další: **krok 10**.
- 2026-10-03: review celého jádra `sim/`: přesné, ale ne čisté (paměť originálu v modelu). Rozhodnuto napsat nové
  jádro `logic/` bez čehokoli ze staré aplikace, ověřené sémantickými replayi `UGR 1`; zadání v
  [rewrite-design.md](rewrite-design.md), kroky N1–N8 výše. Krok 10 až po N8. Další: **krok N1**.
- 2026-10-03: krok N1 hotový - [rewrite-audit.md](../2_reverse_engineering/notes/rewrite-audit.md): měření na `sim/` (větev `audit/n1`, `main` beze
  změny). Přehrávač bez převzetí neznámých hodnot, který po každém snímku „otráví“ všechna pole, jež navržená tabulka
  UGR 1 nepovažuje za definovaná: všech 161 replayů projde, logika zbytky paměti nečte. Sdílená slova originál nikdy
  nečte v jiném významu, animace za koncem nikdy, 16bitové přetečení ani rozdíl neznaménkového porovnání se nestane;
  zvláštnost sondy doleva / nahoru změní výsledek 267×, kapky přes okraj stránky 20 tis.×. Opravy návrhu: `impact`
  a `water.row` nejsou stav, `effort` přežívá mezi pokusy, poloha viděná minulý snímek jen u cestujících s trasou,
  maska 320 px. Další: **krok N2**.
- 2026-10-03: krok N2 hotový - `.\gradlew.bat :extractor:run` zapíše `assets/sim/ugh-data.ugd` (UGD 1, 1,4 MB):
  pravidla, sprity, klávesy, animace, druhy, 81 levelů s plošinami, cestujícími, nepřáteli a maskou 320 × 192, pořadí
  obou režimů; vše přepočtené a pojmenované (`Names.kt`, `LogicData.kt`). `LogicDataTest` porovná každý level obou
  režimů po načtení portem a masky se stránkami pozadí - zelený. Popis v `phase2-data.md`. Další: **krok N3**.
- 2026-10-03: krok N3 hotový - `.\gradlew.bat :verify:replays` zapíše 161 replayů ve dvou formátech: `UGR 0` pro
  `sim/` (CTest dál zelený) a sémantické `UGR 1` (`replays/ugr1/`, `SemanticProjection.kt`: pojmy hry, žádné adresy,
  pole jen definovaná podle tabulky po stavech z N1). Testovací pilot už nezapisuje stav cestujících: místo
  `injectHit` pouští visícího cestujícího před letícího flyera; zásahy jen vrtulník, energie, životy. Finální tabulka
  polí v kap. 9 [rewrite-design.md](rewrite-design.md). `:verify:test` zelený (14 min). Další: **krok N4**.
- 2026-10-03: krok N4 hotový - kostra `logic/` (C++20, CMake + Ninja, `logic/build.ps1`): `units/`, `data/` (čtení
  UGD 1 s kontrolami), `events/`, sezení a náhoda, tok hry po fázi popisku, PC klávesnice, návrh C API
  `include/ugh_logic.h`, knihovna `ugh_logic_replay` (zápis UGR 1) a nástroj `replay_check`. 11 testů, všech 161
  replayů souhlasí v `game.*` až do prvního popisku (`-CheckOptions "--only game. --until game.phase=caption"`).
  Další: **krok N5**.
- 2026-10-03: krok N5 hotový - svět levelu (vrtulníky, plošiny, voda, déšť, energie, fade), fyzika vrtulníku se sondou
  a dotykem spritu, snímek hry, zásahy pilota (`Cheats`). Replaye levelu 1 souhlasí ve hře, vrtulnících a plošinách
  do prvního nástupu (náhodné celé), 142 ze 161 replayů do prvního nástupu; zbylých 19 rozhodí chybějící nepřátelé.
  24 testů. Další: **krok N6**.
- 2026-10-03: krok N6 hotový - cestující (s trasou i stojící), společný balistický pád, bonusy. 6 z 8 replayů levelu 1
  souhlasí ve všem kromě nepřátel celé (týmový dlouhý 26 tis. snímků), zbylé dva do odrazu od stromu. Oprava UGR 1:
  `pad.waiting` je index cestujícího. 28 testů. Další: **krok N7**.
- 2026-10-03: krok N7 hotový - nepřátelé (flyer, walker, foukač, strom) jako samostatné třídy se stavy. **Všech 161
  replayů UGR 1 projde celých** (každé pole, každý snímek, stejná množina polí); `logic/build.ps1`: 32 testů + 161
  replayů, 16 s. Další: **krok N8**.
- 2026-10-03: krok N8 hotový - nové jádro `logic/` je jediné: `sim/`, zápis `UGR 0`, řádky `B` (`StageRecorder`)
  a `ugh-sim.bin` (`Sim.kt`) smazané; masky počítá `Masks.kt`, data jsou v `assets/logic/ugh-data.ugd`, replaye UGR 1
  přímo ve `verify/build/replays`. C API má pohled pro vykreslení (`ugh_logic_get_view`), průvodce
  `logic/README.md`, mapa na originál `logic-map.md`. `/code-review high`: 10 nálezů, všechny opravené. Kontroly
  kap. 12 (grep, jedna třída na soubor, složka = namespace, závislosti jedním směrem) splněné. 35 testů + 161
  replayů za 10 s. Další: **krok 10**.
- 2026-10-03: krok N9a hotový - repo ve složkách podle kroků (`1_original/` … `6_verification/`, `docs/`), Kotlin jen
  přesunutý (`projectDir` v `settings.gradle.kts`, cesty `1_original/` v `.kts`; `:desktop:run` běží v `1_original/`,
  kde `GameFile` najde `UGH.EXE`). `5_remake/logic/build.ps1` staví knihovnu a testy jednotek, `6_verification/build.ps1`
  přidá logiku přes `add_subdirectory`, pole replayů a `replay_check`: 162 testů zelených. README v kořeni a v každé
  číslované složce. Další: **N9b etapa 1**.
- 2026-10-03: N9b etapa 1 hotová - šablony `state/State<Entity, Context>` a `state/StateMachine<Entity, Context>`
  (entita z automatu dědí, `changeState` / `continueIn` / `state()` jen tam) místo 7 kopií; `*State` jsou aliasy,
  jen `WalkerState` a `StandingState` zůstaly potomky (pomocník omráčení, `falls()`). Bonus dostal `enter` (`Lying`).
  Stavy `changeState` dál volají - kontrola „grep mimo `state/`“ tedy platí pro definice. 38 testů + 161 replayů
  zelených. Další: **N9b etapa 2**.
- 2026-10-03: N9b etapa 2 hotová - `world::Figure` (poloha, sprite, animace) je základem `Passenger` i `Enemy`,
  žádná `protected` data. `RoutePassenger` má části `Route` (zastávky, zpoždění), `PassengerCall` (místo čekání,
  volání / mávání), `Ride` (nosič, rychlé doručení: `ride().start(player, frames)`), `Swim` (fyzika šplouchnutí
  a potápění, čas na hladině); stavy volají záměry místo setterů. Časovače nepřátel jsou `Countdown` části
  (`watchTime().start(...)`), letec `flyTowards(side)` místo dvou setterů. 38 testů + 161 replayů zelených.
  Další: **N9b etapa 3**.
- 2026-10-03: N9b etapa 3 hotová - druhy cestujících po jednom: `PassengerKind` (jméno, box) ← `AnimatedPassengerKind`
  (animace, jízdné) ← `RoutePassengerKind` / `SwimmerKind`, a `StandingPassengerKind`; žádný `Type` ani pole platná
  jen pro část typů. Cestující s trasou drží svůj druh na souši a `inWater()`; do vody a zpět `intoWater()` /
  `outOfWater()`, druh ve vodě je dvojice `RoutePassengerKind::swimmer` / `SwimmerKind::land`. V `data/` jsou všechny
  záznamy (druhy, umístění, `Route`, `Rules`, `Animation`, `CollisionMask`) struktury s veřejnými poli, umístění
  navíc s `accept`. 38 testů + 161 replayů zelených. Další: **N9b etapa 4**.
- 2026-10-03: N9b etapa 4 hotová - logika bez DOSu: `Game::key(player, key, pressed)` / C API `ugh_logic_key` pro
  klávesy pilotů a `Game::menuKey` / `ugh_logic_menu_key` (Esc, P, jiná) pro smyčku hry (`input::MenuInput`: poslední
  klávesa platí do další, popisek čeká na novou). `PcKeyboard` a `KeyBinding` jsou v `6_verification/keyboard/`
  (tabulku kláves čte `KeyFile` z dat, logika záznamy `key` přeskočí); před snímkem pošle klávesu smyčky, když se
  poslední skenkód od minulého snímku změnil, což přesně odpovídá čtení originálu. Mrtvá `LONGEST_SEQUENCE` smazaná
  při přesunu. 38 testů logiky + 2 testy klávesnice + 161 replayů zelených. Další: **N9b etapa 5**.
- 2026-10-03: N9b etapa 5 hotová - typy hry místo registrů: jízdné, vzhled, plošiny, životy, energie, časovače
  (`Countdown`) a ostatní hodnoty jsou `int`, `unsignedLess` a `bits()` zmizely; 16bitové přetečení zůstalo jen
  uvnitř `Fixed` a `Speed` (`int` rozhraní, `Int16` uvnitř). Zvláštnosti jsou ve svých třídách (`CollisionProbe`,
  `Copter` pixelová řada po hodu, `Raindrop` stránka 384 px); C API dává jen kapky na obrazovce (`Raindrop::onScreen`).
  Nalezena chyba překladače MSVC: `<=>` nad `int16_t` po negaci a posunu dával špatný výsledek (každý odraz od
  podlahy bral jako od stropu) - `Int16` proto drží hodnotu v `int`. 38 testů + 2 + 161 replayů zelených.
  Další: **N9b etapa 6**.
- 2026-10-03: N9b etapa 6 hotová - prodleva animace walkera jednou (`Walker::animate()`, `FRAME_DELAY`), chůze
  cestujícího jednou (`RoutePassenger::stepTowards(feet)`, i k vrtulníku), stavy na plošině mají společný základ
  `OnPickupPad` (Template Method: voda, vrtulník, pak `walk` / `stay`), zóna foukače je `TouchBox::between`, voda má
  pojmenované dotazy (`copterOnWater()`, `copterOnWaterWithRoom()`, `stillCopterOnWaterWithRoom()`), fáze toku hry
  pracují přes `Attempts` (má ho jen `GameFlow`), na fasádě `Game` zůstalo veřejné API. `LONGEST_SEQUENCE` zmizela
  už v etapě 4. 38 testů + 2 + 161 replayů zelených. Další: **N9b etapa 7**.
- 2026-10-03: N9b etapa 7 hotová - `DataFileReader` (dřív 565 ř.) jen skládá: `UgdTokenizer` (řádky -> `UgdRecord`),
  `RecordReader` (klíče, čísla, rozsahy, chyby s číslem řádku), `KindsReader` (animace, druhy), `RulesReader`
  (pravidla, sprity; místo `KeysReader` - klávesy od etapy 4 čte `6_verification`) a `LevelReader` (levely, plošiny,
  umístění, maska, pořadí); největší soubor 199 ř. `KeyFile` v `6_verification` používá stejný `UgdTokenizer`
  a `RecordReader`. Texty chyb beze změny. 40 testů + 2 + 161 replayů zelených. Další: **N9b etapa 8**.
- 2026-10-03: N9b etapa 8 hotová - kontroly „Hotovo když“: v `5_remake/logic/src` není `scancode`, `unsignedLess`,
  definice `changeState` mimo `state/` ani `protected` data; žádný soubor nad 200 ř. (`CopterPhysics.cpp` 196);
  žádná třída nad ~15 veřejnými metodami (různá jména, const/non-const jednou; nejvíc 16: `RoutePassenger`, `Figure`)
  - proto ještě části `Copter` (`Motion` jako základ, `Rotor`, `Cabin`, `controls()`), `Level` (`Copters` s dotazy
  na vrtulníky, `Delivery`), `Session` (`Score`), `RoutePassenger` (`RouteKinds`). Dokumentace: README logiky
  (pravidla kódu, kam sáhnout), README `6_verification` (vzory, kam sáhnout), `rewrite-design.md` body 12-15,
  `logic-map.md`. `/code-review high`: 7 nálezů, všechny opravené (mj. energie zas jako originál: pod nulou
  16bitově přetéká a doplnění bonusem ji naplní - pojmenovaná zvláštnost v `Energy`; test `PcKeyboard::takeMenuKey`;
  sdílené čtení souboru `UgdTokenizer::tokenizeFile`). Junior test (agent jen s README): plyn vrtulníku
  (`CopterPhysics.cpp` `GRAVITY`) a počet životů (`Session.hpp` `START_LIVES`) jeden soubor; mávání netrpělivého
  cestujícího README neukazovalo - doplněn řádek (`Impatient.hpp` `WAVE_TIME`), testy berou `START_LIVES`.
  40 testů logiky + 3 klávesnice + 161 replayů zelených. Krok N9 hotový. Další: **krok 10**.
- 2026-10-03: N9c etapa 1 hotová - objekty místo indexů: `Copters` vrací `Copter*` (`landedOn(pad)`,
  `onWater...`, `all()`), `TouchBox::firstCopterIn(copters)`, `CopterPhysics::fly(copter)`, nosič cestujícího je
  `Copter*`, vrtulník zná `player()`, plošina `index()`, `Level::pads()`. Cestující s trasou má
  `route().pickupPad(level)` / `targetPad(level)`, walker `pad(level)`; `PassengerContext` a `EnemyContext` dědí
  `PlayContext` (žádné `context.play.`). 40 testů + 3 + 161 replayů zelených. Další: **N9c etapa 2**.
- 2026-10-03: N9c etapa 2 hotová - bez `friend` do cizích vnitřků: měnící se stav hry je `game::GameState`
  (`reset` pro novou hru), `Attempts` a `Cheats` dostanou jen to, s čím pracují (data, stav, posluchač, diagnostika),
  `GameFlow` dostane `Attempts`. Čtenáři dat plní `GameData::Contents`, `GameData(contents)` je pak neměnné. Zbylé
  `friend` jen továrny svých kolekcí. 40 testů + 3 + 161 replayů zelených. Další: **N9c etapa 3**.
- 2026-10-03: N9c etapa 3 hotová - testovací pilot mimo logiku: `Cheats` a metody `*ByTestPilot` zmizely ze
  `src/`; `testing::TestPilot` je knihovna `ugh_logic_testing` v `5_remake/logic/testing/` (testy jednotek
  a `replay_check`), doména ho jen jmenuje jako `friend` (`Motion`, `Copter`, `Energy`, `Session`, `Game`).
  40 testů + 3 + 161 replayů zelených. Další: **N9c etapa 4**.
- 2026-10-03: N9c etapa 4 hotová - balíčky: `data/` (jádro: `GameData`, `Rules`, `SpriteIds`, `Animation`, `Box`,
  bonusy), `data/kinds/` (druhy cestujících a nepřátel), `data/levels/` (definice levelu, plošina, maska, vítr,
  trasa, umístění a jejich Visitory), `data/ugd/` (čtení souboru UGD 1); namespace `levels`, ne `level`, ať se nebije
  s proměnnými. `PlayerKey` v `input/`. 40 testů + 3 + 161 replayů zelených. Další: **N9c etapa 5**.
- 2026-10-03: N9c etapa 5 hotová - čtení dat DRY: každý čtenář ví, které typy záznamů čte (`reads`, tabulka typ ->
  metoda, `RecordTable`), a má své `readAll`; `DataFileReader` je jen skládá (neznámý záznam = žádný
  čtenář ho nečte). `LevelReader` má metodu na každou část levelu (dřív 75řádkový if-else), plošinu čte po jménech,
  `RecordReader` čte aktuální záznam (bez `r` v každém volání). Z `KindsReader` se oddělily `AnimationsReader`
  a `PassengerKindsReader`, z `LevelReader` umístění (`PlacementReader`); každý soubor pod 200 ř. Texty chyb beze
  změny (u souboru s více chybami může být první jiná: pravidla se teď čtou celá před levely). 40 testů + 3 + 161 replayů zelených.
  Další: **N9c etapa 6**.
- 2026-10-03: N9c etapa 6 hotová - nepřátelé bez `Countdown&` ven: záměry (`startWatching` / `watchOver`,
  `startResting` / `restOver`, flyer jeden odpočet `wait` / `waitOver` pro skrytí i křik - stavy se vylučují);
  omráčení walkera a foukače je část `enemies::Stun` s jedinou `Stun::TIME`. 40 testů + 3 + 161 replayů zelených.
  Další: **N9c etapa 7**.
- 2026-10-03: N9c etapa 7 hotová - C API `ugh_logic_default_settings()` (frontend nemusí znát řádek deště ani
  semínko; popis polí v `ugh_logic.h`); převody jednotek pojmenované: `Fixed::half()`, `Speed::twicePerFrame()`,
  `* 32` / `* 2` místo posunů doleva. Zbylé posuny jsou aritmetika originálu s komentářem (odraz, polovina
  a čtvrtina rychlosti, generátor náhodných čísel). 41 testů + 3 + 161 replayů zelených. Další: **N9c etapa 8**.
- 2026-10-03: N9c etapa 8 hotová - dokumentace: README logiky (moduly po balíčcích `data/kinds` ← `data/levels` ←
  `data` ← `data/ugd`, `testing/`, vzory Table of methods a test peer, pravidla „objekty, ne indexy“, „žádný
  `friend` do cizích vnitřků“, kam sáhnout), README verifikace, `rewrite-design.md` (pravidla 16-18, výjimka
  z pravidla 8 pro `RecordTable`, strom modulů). Při kontrole ještě: `Animation`, `AnimationPair`, `Box`,
  `BonusKind` přešly do `data/kinds` (bez kruhu mezi složkami dat), `RecordTable` zvlášť od `RecordReader`,
  `walker::Charge`, `route().pickupPad(level)` (nejvíc 16 veřejných metod: `RoutePassenger`, `Figure`), includy
  seřazené. `/code-review high`: 7 nálezů, 5 opraveno, 2 záměrně ponechány (podpis tabulky; `type` cestujícího je
  hodnota pole). Junior test: 2 ze 3 požadavků jen z README, třetí (doba ležení bonusu) doplněn do „kam sáhnout“.
  41 testů + 3 + 161 replayů zelených. Krok N9c hotový. Další: **krok 10**.
- 2026-10-03: N9d hotový (review po N9c) - objekty místo indexů i tam, kde zbyly: vrtulník stojí na `const Pad*`,
  na plošině čeká `const Figure*`, walker a trasa cestujícího drží plošiny levelu (`walker.pad()`,
  `route().pickupPad()`); index entity je v `world::Figure` (cestující, nepřítel i bonusový předmět - ten teď z
  `Figure` dědí). `Copter` skládá `Motion` (už nedědí), životy jsou `world::Lives`; testovací pilot je jediný
  `friend` fasády `Game` (dřív i `Copter`, `Motion`, `Energy`, `Session`). Rychlosti pádu a výpadu walkera jsou
  `Fixed`, ne holý `int`; ruční odpočty (`arrivalDelay`, `quickDeliveryTime`) přes `Countdown::tickToZero`.
  `passengers::route::Route` přejmenována na `RouteProgress` (kolize s `data::levels::Route`); stejná jména stavů
  v různých balíčcích zůstávají (jsou to jména stavů v replayích). Pole replayů v `6_verification` z tabulek
  (`FieldTable`) místo řetězů `if`. Opravené komentáře (`Raindrop`, `RecordReader`), pravidla 19-22 v
  `rewrite-design.md`. 41 testů + 3 + 161 replayů zelených. Další: **krok 10**.
- 2026-10-03: N9e hotový (review po N9d; 3 kola: agent opraví, nový agent zreviduje, konec až bez konkrétní
  vady) - jedno místo pro každý fakt: tvar vrtulníku `world::copter::CopterShape`, rozměr obrazovky
  `data::levels::ScreenSize`, počet obtížností, `Level::crash`/`fadeOut`; `world/` rozdělen na `session/`,
  `figure/`, `scenery/`, `copter/`; žádné bool parametry; `PassengerForm`, `PickupWait`, odpočty `tick...`;
  `static_assert` pořadí událostí C API; čtečka dat hlídá druhy nepřátel, `rules` a `sprites` právě jednou;
  pravdivé komentáře a rozsahy v `ugh_logic.h`, pojmenované quirky (`Splash`, `floatOnSurface`, box nepřítele).
  42 testů + 163 kontrol replayů zelených. Další: **krok 10**.
- 2026-10-03: krok 10 hotový - `5_remake/game/` (UE 5.8, bez map a assetů: scénu staví
  `AUghGameMode` z kódu). Modul `UghLogic` překládá zdrojáky `logic/src` na místě (Build.cs generuje obal na každý
  .cpp, UBT nesnese stejná jména souborů jako `Falling.cpp`), C API exportované (`UGH_LOGIC_EXPORTS/IMPORTS`); v
  editoru i testy `Ugh.Replays.*` přes knihovnu `6_verification/check/` (přesun z `tools/replay_check`). C API má
  pozadí levelu (`ugh_logic_pad_count`, `ugh_logic_get_pad`, `ugh_logic_solid`) a hodnoty pro frontend (obrazovka,
  tělo vrtulníku, plná energie, plně zobrazený level, snímky za 1000 s); diagnostiky po každém kroku zahodí.
  Frontend `UghGame`: pevný tik 70,086 Hz + interpolace, klávesy (šipky + pravý Ctrl / mezerník, pilot 2 W A S D,
  Esc, P), maska slitá do kostek, plošiny s čísly, voda, postavy velikosti spritů, HUD, DLSS/FSR/TSR (U) a frame
  generation (G). Slunce bez stínů (na natažených kostkách dělají pruhy). Skripty `setup.ps1` (pluginy z UghTrial,
  mimo git), `build.ps1`, `test.ps1`, `shot.ps1` (level 1 sám bez okna do PNG), `play.ps1`. Review agentem: 8 nálezů
  opraveno. 43 testů + 3 + 161 replayů (CMake) a 161 replayů v UE zelených, Jan level 1 odehrál
  (`play.ps1`). Další: **krok 11**.
- 2026-10-03: krok 11 - vizuální směr „Pravěké dioráma“ (`docs/visual-concept.md`), level 1 hotový (bez Janova
  ručního odehrání). Útes v řezu z kolizní masky (`FUghRockMesh`: řez, podlahy, stropy a stěny po pixelech, hrbolatá
  zadní stěna jeskyně), obarvený původní kresbou levelu složenou za běhu z dlaždic (`FUghLevelArt`, data jen z
  `assets/`), průsvitná voda, mlha, dřevěný rám, ohniště na nejdelší suché římse bez plošiny (zhasne pod vodou),
  postavy z plastelíny, bubliny cestujících jako kartičky s původním spritem; světla, mlha a kamera v `AUghStage`
  (slunce bez zabarvení atmosférou, expozice -1). Materiály dělá commandlet `UghMakeAssets` (modul `UghEditor`) při
  `build.ps1` do `Content/Generated` - v gitu žádné binární assety. Autopilot screenshotu `FUghShot` (vznášení,
  `-UghShotAt=`). Balení `package.ps1` (UAT přes `cmd`, `NO_PROXY += ::1`, data vedle hry, zip bez `.pdb`, 609 MB) a
  `pso.ps1` (nahrání PSO zabalenou hrou, expand jen s klíči `PCD3D_SM6` - smíchané SM5 shodí cook, druhé balení):
  47 PSO předkompilováno při startu. Review agentem: 8 nálezů opraveno. 43 testů + 3 + 161 replayů (CMake) a 161
  replayů v UE zelených. Další: Jan odehraje level 1 (`play.ps1`), pak **krok 12** (zatím nenaplánovaný).
- 2026-10-03: krok 12 hotový (bez Janova ručního odehrání v okně) - menu před hrou (`FUghMenu`: jeden hráč / tým,
  obtížnost, heslo z `assets/levels.json` vč. číslic `1983`; za menu ztlumené dioráma levelu, který by spustilo; konec
  hry zpět do menu s výsledkem), test `Ugh.Menu`. C API má `ugh_logic_view.wind`. Bouřka ve větrných levelech (slabší
  chladné slunce, hustá šedá mlha), plamen ohniště se kloní po větru, kapky jako plastelínové čárky šikmo po větru;
  tmavý pruh pod stavovým řádkem (byl nečitelný přes skálu nahoře). Autopilot `FUghShot` startuje levely z menu klávesami
  hráče (režim, heslo, Enter) a po snímku hru vzdá: `shot.ps1 -Level <n> [-Team]`, `levels.ps1` (všech 69 + 81 levelů
  za 22 min, kontaktní archy `Saved\Shots\Levels\levels-1p.png` / `levels-team.png`) - v přehledu žádný level nemá vadu
  (ohniště, voda i stoupající voda, okraje). Review agentem: 6 nálezů opraveno, druhé kolo čisté. CTest logiky
  a `6_verification` (163, s 161 replayi) a 162 testů v UE zelené. Další: Jan odehraje menu v obou režimech
  (`play.ps1`), pak **krok 13**.
- 2026-10-04: krok 13 hotový (bez poslechu) - zvuk a hudba originálu. `.\gradlew.bat :extractor:sound`
  (`extractor/Sounds.kt`, `SoundRecorder.kt`) pustí každý blok ADLX knihovnou zvuku Kotlin portu tak, jak ho hra hraje
  (hlasitost, jednou / smyčka), a OPL2 portu ho vyrenderuje do `assets/sound/*.wav` (49 716 Hz, 16 bit mono, zesílení
  8× jako přehrávač portu): 7 efektů (znělka titulku, „bad luck“ po prohře, křik letce, mávání, foukač, bonus,
  puštěný cestující) a 3 hudby (menu 65 s, level 108 s, konec hry 104 s). Smyčky (hudba, mávání) jsou druhý průchod
  stopou, takže navazují bez švu. Podle portu má zvuk jen 8 z 15 událostí logiky (titulek, křik, mávání start/stop,
  foukač, strom a rychlé doručení = bonus, puštěný cestující); náraz, nástup, zaplacení, voda, omráčení, sebraný
  bonus a dokončený level originál neozvučil; mávání cestujícího žádný zvuk nemá. UE: `FUghSimulation` sbírá události,
  `FUghSounds` (tabulka událost -> soubor, čtení WAV), `FUghMixer` (hudba se smyčkou, zpožděním a stmíváním, 4 kanály
  efektů jako originál, smyčka do zastavení entitou, hlasitost), `FUghSoundPlayer` (menu: hudba menu; hra: hudba
  levelu od začátku hraní, na konci efekty stop a hudba se stmívá s obrazem 1,7 s; prohra: „bad luck“ a pak hudba
  menu; všechny levely: hudba konce), `AUghSpeaker` (procedurální zvuk krmený každý snímek ~60 ms dopředu). Hlasitost
  PgUp/PgDn (i v menu, v HUD). Bez souborů hra mlčí a zaloguje to (ověřeno shotem bez `assets/sound`), autopilot
  a `-nosound` mlčí. Rozdíl od originálu: efekty neberou hudbě hlasy (hrají obě). Ověření bez poslechu: `SoundsTest`
  (délky, RMS 235-3182, špičky -8 až -26 dBFS, bez ořezu, průchody smyček stejně dlouhé a stejně hlasité 0,999,
  šev bez skoku), `SoundRenderTest` (hudba menu proti celému programu portu: korelace 0,995, dva průchody portu mezi
  sebou 0,992 - liší se jen fází LFO), UE testy `Ugh.Sounds.Files/Mixer/Player`. CTest logiky, `6_verification`
  (163) a 165 testů v UE zelené. Čeká na Jana: poslech (`play.ps1`: menu, level s letcem a foukačem, prohra,
  hlasitost). Další: **krok 14**.
- 2026-10-04: krok 14 hotový - knihovna 3D assetů a cesta do UE. `5_remake/game/Assets.json` (24 assetů: id, druh,
  zdroj, stránka, autor, licence CC0, cesta) a `fetch-assets.ps1` (stáhne do `assets/3d` jen chybějící, Poly Haven
  přes API s MD5, ostatní přímé odkazy se SHA-256 z manifestu, archivy rozbalí `tar.exe`, rozpočet 100 GB, tabulka na
  konci; dnes 0,57 GB): textury skal `cliff_side`, `rock_face_03`, `rock_wall_02`, `mossy_rock`, `lichen_rock`, hlína
  `red_laterite_soil_stones`, kůra `palm_bark` (Poly Haven 2k), tráva `grass004` a mech `moss002` (ambientCG), oblohy
  HDRI `belfast_sunset_puresky` a `kloofendal_48d_partly_cloudy_puresky` (4k), modely balvanů, kamenů, stromu džungle
  `island_tree_02`, kapradiny, keře, rostlin džungle, trávy, kmene (Poly Haven glTF), palmy Nobiax (OpenGameArt; OBJ
  převede `Blender/palm.py` přes `blender -b` na glTF s vyříznutými listy) a ohniště (Kenney Nature Kit FBX).
  Quaternius nemá přímé odkazy (Google Drive / itch), Poly Haven nemá palmu. Commandlet `UghImportAssets` (modul
  `UghEditor`, Interchange) po materiálech v `build.ps1` naimportuje vše do `Content/Imported/<id>` (gitignore):
  modely jako Nanite sítě s materiály glTF, sady textur s nastavenou kompresí a instancí `MI_<id>` nového master
  materiálu `M_UghPbr` (`UghMakeAssets`: BaseColor, Normal, Roughness = G, Occlusion = R, Tiling); idempotentní
  (`Import.stamp`: soubory, z kterých vznikl; nový import nejdřív smaže starý), chybějící asset přeskočí s řádkem
  v logu; `-ForceImport`. Ve hře registr `UghAssets.h` (id -> statické sítě přes asset registry) a důkaz: `AUghScenery`
  postaví v každém levelu palmu a kde se vejdou 1-2 kameny (`UghDecorations`, místo na římsách `UghLedges`, kam se
  přesunulo i hledání ohniště) - na suché římse s místem pro celý box, mimo plošiny, za deskou hry, stejné pro stejný
  level; bez assetů tytéž dekorace z plastelíny (ověřeno s přejmenovaným `assets/3d` i `Content/Imported`: build,
  shot i 166 testů v UE). Plošiny pokrývají skoro celé římsy, takže kde jinde místo není, stojí palma na římse plošiny
  (aspoň 28 px, koruna nad cedulí s číslem), kameny jen mimo plošiny (v 82 ze 150 levelů). Test `Ugh.Scenery`
  (každý level má palmu, vše na zemi s místem, mimo plošiny, za deskou, deterministicky).
  `levels.ps1` (21,6 min) a archy bez vad, palma je vidět (ve stínu jeskyně tmavá - světlo a materiály krok 15/18).
  CTest logiky, `6_verification` (163) a 166 testů v UE zelené s assety i bez nich. Pro krok 15: textury skal jsou
  instance `MI_<id>` s mapou výšky (`height`, TC_Grayscale) pro displacement, oblohy HDRI jako 2D HDR textury
  (long-lat), `Tiling` je opakování na UV jednotku (rozměry textur Poly Haven 1,8-3 m). Balení s `Content/Imported`
  zatím nevyzkoušené (`DirectoriesToAlwaysCook` přidáno). Další: **krok 15**.
