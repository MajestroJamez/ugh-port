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
- Snímky (Jan 2026-10-05): po každém kroku jen rychlá sada `levels.ps1 -Quick` (12 levelů se všemi náladami a jevy na
  jednom archu `levels-quick.png`, asi 3 min); celých 150 levelů `levels.ps1` jen jednou denně v noci nebo před
  milníkem.

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
- Po každém kroku: testy logiky, `6_verification` a UE zelené, `levels.ps1 -Quick` a kontrola archu (celý
  `levels.ps1` jednou denně v noci nebo před milníkem), zápis do Stavu, commit a push (v noci 2026-10-04 předem
  schváleno Janem).

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

## Krok 18b - Fotorealistický svět z Electric Dreams

Jan hru vyzkoušel: vypadá jako hra z GameMakeru. Chce moderní, skoro fotorealistický vzhled (jako level dnešního Unreal
Tournamentu): opravdu naskenované skály, fotorealistické palmy s velkými listy, kořeny prorůstající skálou. Skripty
z Blenderu a CC0 assety nestačí. Zdroj: Epicův vzorek **Electric Dreams Environment** (Megascans: džungle, skály,
rostliny, stromy, kořeny, mech; licence pro použití v projektech Unreal Engine), stažený v
`C:\Users\Ja079591\IdeaProjects\Unreal Projects\ElectricDreamsEnv` (UE 5.8, ~56 GB) - jen ke čtení, nikdy se neukládá.

- **Inventura:** v `ElectricDreamsEnv/Content` najít skály, útesy a balvany (Nanite), materiály skal, kořeny,
  kapradiny, rostliny s velkými listy, palmy, mech, trávu, liány; seznam s velikostmi v poznámkách kroku.
- **Přenos:** skript, který zkopíruje jen vybrané assety a jejich závislosti do `Content/External/ElectricDreams`
  (gitignore, nikdy v gitu; jak ho znovu vyrobit, je v README), celkem pár GB.
- **Použití:** hrana skály v rovině hry zůstává přesně na kolizní masce (test `Ugh.Rock`), ale čelo a zadní stěna
  dostanou naskenovanou skálu (Nanite skály podél obrysu za deskou hry a v plné skále, naskenované textury na síti
  skály), kořeny prorůstající skálou, rostliny Electric Dreams místo CC0 / Blenderu, kde jsou lepší (palmy s velkými
  listy, kapradiny, mech, liány); pravidla chráněných objemů kroku 18 (test `Ugh.Scenery`) platí dál, bez externího
  obsahu se repo přeloží a testy projdou.
- **Světlo a post:** Lumen, dobrá expozice, barevné ladění, jemná mlha - fotorealisticky, ne tma.
- Hotovo když: snímky jsou proti kroku 18 jasně fotorealističtější (1p-01, 1p-03, 1p-43, team-21), archy
  `levels.ps1` zkontrolované, testy zelené, fps v logu (Radeon 890M, medián aspoň 25), dokumentace a zápis do Stavu.

## Krok 18c - Fotorealistická čelní skála

Po kroku 18b vypadá zadní stěna jeskyně jako naskenované útesy, ale čelo skály (síť `FUghRockMesh`, materiál
`M_UghCliff`) působí jako plochý světlý vápenec nebo omítka: bez velkých skvrn, reliéfu, vrstev a skoro bez barev,
tráva a mech na horních plochách se ztratily. Je to největší plocha na obrazovce, rozhoduje o vzhledu hry.

- Čelo jako naskenovaná skála ladící s pískovcovými útesy vzadu: naskenované povrchy vzorku ve světových souřadnicích
  (triplanár) s velkými skvrnami proti opakování, vrstvy míchané podle výšky, tmavší spáry a dutiny, mokrá tmavá skála
  u vody, mech a tráva na horních plochách a přes horní hrany, hlína na vodorovných plochách; teplá okrová a šedá
  s tmavšími vrstvami.
- Víc reliéfu sítě jen mimo desku hry (hloubka -20 .. 20 zůstává maska, test `Ugh.Rock`).
- Fallback bez externího obsahu zůstává, výkon `levels.ps1` medián aspoň 25 fps.
- Hotovo když: snímky před a po (1p-01, 1p-03, 1p-43, team-21) ukazují čelo jasně jako fotorealistickou skálu s trávou
  a mechem nahoře, archy `levels.ps1` zkontrolované, testy zelené, dokumentace a zápis do Stavu.

## Krok 18d - Fotorealistické postavy (MetaHumans)

Jan: postavy z Blenderu jsou špatné, chce fotorealistické lidi. Stáhl Epicův vzorek MetaHumans
(`..\Unreal Projects\MetaHumans`, UE 5.7, jen ke čtení) a do UE 5.8 nainstaloval „MetaHuman Creator Core Data“.

- Pilot mladý pravěký muž, cestující podle vzhledu originálu (`cargo_look` 1-3): obyčejný muž, žena, stařec; vzhled 4
  zůstává kámen s očima. Pravěký vzhled: rozcuchané vlasy, vousy u mužů, kožešina nebo kůže; předlohy a úpravy
  (věk, vlasy, vousy) skriptem v MetaHuman Creatoru, kde to bez okna jde, jinak hotové postavy vzorku.
- Přenos jako v 18b: jen vybrané postavy do `Content/External/MetaHumans` (gitignore), pluginy MetaHumanů zapnuté
  v projektu (závislosti mimo `/Game` jen z pluginů); Substrate a bez virtuálních textur (18b).
- Akce `FUghFigureActions` / `EUghCaveAction` (stání, chůze, mávání, sezení v kabině, šlapání, visení, šlapání vody,
  plavání, pád): klipy, kde jsou (vzorek, Creator, Manny šablony), jinak pózy; velikost podle spritů, hloubka v desce
  hry (-20 .. 20, vrtulník ±45), nízké LOD a bez vláken vlasů (`levels.ps1` medián aspoň 25 fps na Radeonu 890M).
  Bez externího obsahu jeskynní muž z kroků 16-17 (repo se přeloží, testy projdou).
- Hotovo když: detaily (`shot.ps1 -Cargo 1/2/3 -CloseUp`, `-Frame`) ukazují fotorealistického pilota a cestující
  při chůzi, čekání a v kabině, archy `levels.ps1` zkontrolované, `Ugh.Figures.*` a všechny testy zelené, dokumentace
  a zápis do Stavu.

## Krok 19 - Voda, déšť, obloha, světlo

- Voda jako moderní vodní plocha (Single Layer Water: vlny, lom, pěna u skály) se stoupající hladinou podle logiky,
  déšť a mlha jako Niagara podle větru, mraky a obloha, nálada levelu (den, soumrak, noc) podle čísla levelu, Lumen.
- Hotovo když: archy bez vad, ve větrných levelech déšť ve směru větru, voda přesně na `water_level`.

## Krok 19b - Lidé: listy, vlasy, sezení

Jan 2026-10-05: místo leopardích triček zakrytí intimních partií velkými listy (Electric Dreams), skutečné vlasy
a vousy (karty, ne zapečené čepice), pilot i cestující v kabině oba sedí a jsou stejně velcí jako při chůzi; veze-li
vrtulník kámen (look 4) a nikdo jiný nesedí, sedí na sedadle zmenšený kámen.
- Hotovo když: detaily (`shot.ps1 -Cargo 1/2/3/4 -CloseUp`) i archy bez vad, testy zelené, fps medián >= 25.

## Krok 19c - Skály podle reference: šedý krasový vápenec, vchody do jeskyní

Jan 2026-10-05: pískovec působí „lakovaně“; skály mají vypadat jako na jeho fotce vchodu do jeskyně
(`C:\Users\Ja079591\.claude\ugh-cave-reference.jpg`: šedý zvětralý vápenec, ostré lomy, mech a lišejník, tmavý
vchod). Naskenované materiály a kameny (Electric Dreams, Poly Haven, ambientCG). Vchody do jeskyní (sprite dveří, odkud
vycházejí cestující) jako skutečné skalní portály s tmou uvnitř, ne díra do skály.
- Hotovo když: snímky srovnané s referencí, `Ugh.Rock` zelený, archy bez vad.

## Krok 19d - Cedule s čísly a bubliny

- Cedule s čísly plošin jako 3D (vyřezané do dřeva nebo kamene) místo pixelového spritu; bubliny cestujících ostré
  (vektorové / UI s číslem plošiny nebo otazníkem), ne kostičkované.
- Zobáček bubliny (Jan 2026-10-05): dnes je vidět na obě strany a vypadá divně. Originál měl jen jeden zobáček podle
  toho, z které strany vůči hráči se bublina ukazuje, a mířil k hráči (vrtulníku). Zjistit z originálu / Kotlin portu,
  jak se strana vybírá (sprite bubliny, poloha vůči vrtulníku), a kreslit jen ten jeden.
- Hotovo když: každá cedule a bublina čitelná v archu, test, že každá plošina má cedulku se správným číslem, a test, že
  zobáček míří ke správné straně.

## Krok 19e - Úvod levelu: let nad mořem ke kameni

- Při popisku levelu kamera jako dron přiletí nad mořem s vlnami k velkému kameni; level je do kamene vytesaný;
  kamera zabrzdí tak, že je vidět level i kus kamene kolem, a pak začne hra (do kamene nevlétá). Herní rovina, kamera
  hry a logika beze změny.
- Hotovo když: snímky z průletu (začátek, střed, konec), autopilot `shot.ps1` / `levels.ps1` dál funguje.

## Krok 19f - Pramen, potok, můstek a vodopád

Jan 2026-10-05: v některých levelech ze skály z díry vyvěrá voda, teče přes cestu (římsu), přes ni vede můstek, a do
popředí padá jako vodopád do vody dole, kde dělá vlny - jen vizuálně, bez vlivu na fyziku a logiku. Vybrat levely, kde
na to je místo (deterministicky), nic nesmí zakrýt postavy, plošiny ani cedule; můstek nesmí měnit herní rovinu (je
dekorace na místě, kde je v masce pevná římsa).
- Hotovo když: aspoň v několika levelech pramen, potok, můstek a vodopád s pěnou a vlnami dole, `Ugh.Scenery`
  a `Ugh.Rock` zelené, archy bez vad.

## Krok 19g - Oheň a louče

Jan 2026-10-05: pěkné ohniště s realisticky udělaným hořením a louče, které osvětlují okolí; plamen má mihotavým
světlem barvit a rozpohybovat okolí (stíny a odlesky na stěnách).
- Hoření z pluginu Niagara Fluids (UE 5.8, simulace ohně a kouře) zapečené do animované textury (flipbook), aby běželo
  levně; ohniště z kamenů a polen s jiskrami a kouřem; louče na stěnách u jeskynních vchodů a v jeskyních; mihotavé
  bodové světlo (jas, barva a mírně poloha), Lumen GI barví okolí, hlavně v nočních a soumrakových levelech. Pokud by
  zapečený oheň nestačil, Jan najde zdarma oheň ve Fabu.
- Hotovo když: detailní snímky ohniště a louče ve dne i v noci, oheň pod vodou zhasne, `Ugh.Scenery` zelený, fps
  quick setu bez poklesu.

## Krok 19h - Nepřátelé a kámen fotorealisticky

Jan 2026-10-05: strom s obličejem, kamenný cestující a nepřátelé jsou ještě staré gumové modely z kroku 17.
- Strom: skutečný strom z Electric Dreams (kůra, koruna s ovocem) s obličejem vyřezaným do kůry, animace jako dnes.
- Kamenný cestující: naskenovaný kámen s mechem a očima (i zmenšený na sedadle vrtulníku).
- Foukač: T-rex od Jana (Fab, `assets/3d/fab/trex`, statický model bez kostry) ležící a spící: kostra a animace
  v Blenderu (dýchání ve spánku, nadechnutí a odfouknutí při foukání, otřesení při omráčení), zavřená tlama.
- Walker: triceratops od Jana (`assets/3d/sketchfab/triceratops`, Unity FBX 2,3 tis. vrcholů, kostra a animace walk,
  run, attack1, die, eat, idle) zachovat, ale vylepšit v Blenderu: opravit měřítko importu, zjemnit síť
  (subdivision + tvar), detailnější kůže (normálová mapa šupin, PBR textura ve vyšším rozlišení), mapovat jeho
  animace na stavy walkera.
- Pterodaktyl: přepracovat v Blenderu se skutečnou texturou kůže a blan křídel (realistický zdarma není).
- Hotovo když: detailní snímky každého nepřítele a kamene, `Ugh.Figures.*` zelené, quick set bez poklesu fps.

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

## Krok 24 - Janovy připomínky po prvním hraní (2026-10-06)

Jan zahrál level 1. Kroky 24a-24d jdou postupně (sdílejí build a editor), každý svým agentem, snímky jen offscreen.

### Krok 24a - Zadní strana vrtulníku kostičkovaná (priorita)

- Zadní část vrtulníku se kreslí „kostičkovaně“ (bloky, schody). Najít příčinu (rozlišení/TSR na průsvitném nebo
  tenkém materiálu, chybějící velocity/depth u průsvitných, Nanite/LOD, maskovaný materiál s ditherem, stín
  s nízkým rozlišením, normály) a opravit u zdroje, ne zamaskovat.
- Hotovo když: snímek zblízka zezadu i z herní kamery v pohybu bez kostiček, ostatní snímky beze změny.

### Krok 24b - Vrtulník: šlapání a sezení

- Nový design: musí být na první pohled vidět, že pilot **šlape** (pedály, kliky, řetěz/převod na vrtuli, nohy se
  točí s vrtulí). Pilot nesedí na sudu, ale na sedátku; pasažér má **vlastní židli** (kamenná/dřevěná, pravěká).
- Hotovo když: snímky letu a přistání ukážou šlapání a obě sedátka, animace nohou svázaná s otáčkami vrtule.

### Krok 24b2 - Vrtulník průhlednější a zelenější (Jan po 24b)

- Zadní stěna: místo proutěné stěny jen dva bambusy křížem omotané břečťanem, jinak průhledné (vidět, co je za
  vrtulníkem). Pozor na 24a: žádné maskované díry v jedné ploše (rozbíjely VSM stíny) - průhlednost geometrií.
- Bambusové sloupky po stranách omotat břečťanem; viditelné liány visící z rámu; celé zelenější. Kožené lemy
  v barvě hráče (rozlišení vrtulníků v týmu) mohou zůstat, ale ne tak dominantní.
- Hotovo když: snímky zblízka a z herní kamery, v týmu oba vrtulníky rozlišitelné, stíny bez kostiček.

### Krok 24c - Čitelnost: méně přesvětlené, postavy vyniknou

- Scéna je přesvětlená a postavy (čekající cestující, piloti, nepřátelé) splývají s pozadím; v pozdějších levelech
  s víc postavami si hráč čekajícího nevšimne. Snížit expozici/kontrast pozadí a postavy oddělit: rim/fill světlo
  jen na postavách, jemný obrys nebo stín za postavou, sytější barvy oblečení proti šedé skále, čekající cestující
  výrazněji (např. jemná záře/kužel světla nad plošinou, ukazování rukou). Gameplay rovina má mít jasnou hierarchii:
  postavy > plošiny > skála.
- Hotovo když: na archu rychlé sady jsou všechny postavy vidět na první pohled ve všech náladách (den, noc, déšť).

### Krok 24d - Let ke kameni plynulý, okraje kamene, měkký náraz do okraje

- Let má na dvou místech škubnutí: najít (změna spline/rychlosti, přepnutí kamery, načtení assetu, hitch kompilace
  shaderu) a odstranit; dojezd ke startu levelu velmi plynulý (ease-out bez skoku na konci).
- Koncová kamera ukáže kolem levelu i okraje okolního kamene (hráč vidí stěny), ne jen samotný level.
- Okraj obrazovky: v originále vrtulník na kraji obrazovky jen zastaví, bez nárazu a ztráty života. Ve 3D to má
  vypadat jako měkký kraj: po stranách levelu závěsy lián/mechu/keřů na kameni; když vrtulník narazí na okraj, porost
  se prohne a zašustí (pár listů opadá), vrtulník se zastaví jako v originále. Logika beze změny.
- Horní okraj (Jan): v originále mohl vrtulník vyletět úplně mimo obrazovku nahoru a náraz do stropu ani v plné
  rychlosti nebral život. Teď bude kámen nad levelem vidět, takže vrtulník nad levelem nesmí vypadat, že vletěl do
  skály: nahoře hustý převis porostu (liány, kapradí, kořeny visící z převisu), do kterého vrtulník zajede a zmizí,
  porost se rozhrne a zašustí; náraz do stropu vypadá měkce. Logika beze změny.
- Hotovo když: záznam letu (snímky po krocích nebo měření kamery po snímcích) bez skoků, koncový snímek s okraji
  kamene, snímek vrtulníku u okraje s prohnutým porostem.

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
- 2026-10-04: krok 15 hotový - útes a jeskyně ve 3D (`docs/visual-concept.md`). Skála je pole hustoty ve 3D
  (`FUghRockField`: mřížka středů pixelů 48 px do stran a 24 px nahoru a dolů za obrazovku, 43 vrstev od čela -8 px po
  zadní stěnu 82 px), jeho povrch surface nets (`UghSurfaceNets`) s normálami z pole, 300-400 tisíc trojúhelníků za
  0,2 s, za běhu postavený jako statická síť (`FUghRockMesh::ToStaticMesh`: stíny VSM a ray tracing se cachují; s
  `ProceduralMeshComponent` se síť kreslila do stínů každý snímek, polovina času snímku). V desce hry je pole vzdálenost
  od hranice masky (`FUghRockOutline`, EDT Felzenszwalb), hrana leží přesně na hranicích pixelů, síť uřízne jen rohy
  schodů. Test `Ugh.Rock`: síť každého z 81 různých levelů řezaná v rovině hry a pixel před ní a za ní - střed každého
  pixelu na své straně, žádný bod (4 x 4 na pixel) dál než 0,5 px od pixelu svého druhu (naměřeno nejvýš 0,12 px);
  posun hrany o 0,7 px test shodí. Před deskou čelo se zaoblenými hranami, za ní převisy (stěny a stropy lezou do
  jeskyně), drsnost rostoucí s hloubkou, podlahy ve výšce masky, zadní stěna 30-78 px (hlubší za tmavými dírami
  kresby), krápníky a spadané kameny (`UghRockFeatures`), za okraji obrazovky se útes zavírá (s šumem, na okraji
  mřížky jistě) a neviditelný „plášť“ kolem mřížky vrhá stín útesu, který pokračuje (bez něj svítilo slunce přes okraj
  mřížky pruhy na zadní stěnu). Dřevěná krabice zmizela, voda pokračuje přes celou šířku. Materiál `M_UghCliff` (HLSL
  custom node v `Source/UghEditor/Shaders/UghCliff.hlsl`, čte ho `UghMakeAssets`): triplanár pěti sad textur z kroku
  14 (`cliff_side`, `rock_face_03`, `grass004`, `moss002`, `red_laterite_soil_stones`) podle směru plochy a
  změkčené kresby, výškové mapy rozhodují přechody, barvy vrcholů nesou AO a hloubku; `M_UghPbr` má výšku zapojenou
  jako parallax (import ji dává do `MI_<id>`). Cedule s čísly plošin (dlaždice 85-90) jsou kartičky se spritem za
  deskou, bubliny cestujících kartičky před čelem skály. Světlo: slunce 45° zepředu shora 8 lux, obloha HDRI na kopuli
  (`M_UghSky`, sky light ji snímá), pevná expozice EV100 2, bouřka tmavší zamračená obloha. Palmy nebyly šedé světlem:
  materiály modelů (instance glTF materiálů enginu) nepovolují Nanite a hra kreslila výchozí materiál - import je
  přepojí na kopie v `Content/Imported/_Masters` (`UghNaniteMaterials`). Snímky logují fps (Radeon 890M, 1280 x 720,
  FSR 67 %): jednotlivě 50-57 (krok 14 asi 75), v `levels.ps1` se notebook zahřeje, medián 33 (krok 14: 45). Bez
  assetů (přejmenované `Content/Imported`) skála v barvách kresby a obloha enginu, shot prošel. `levels.ps1` (28,5 min)
  a archy bez vad, prohlédnuté i snímky (bouřka, stoupající voda 1p-43 po 25 s, tým). CTest logiky, `6_verification`
  (163) a 167 testů v UE zelené. Pro krok 16: skála před deskou hry sahá do -60 jednotek (`FUghRockMesh::FrontDepth`),
  bubliny a karty stojí před ní (-63), vrtulník v desce (-20 .. 20) skálu neprotne jen tam, kde je vzduch masky - část
  modelu před deskou (kokpit, pilot) by nad skálou mohla zajet do čela; voda začíná těsně před čelem (-61). Další:
  **krok 16**.
- 2026-10-04: krok 16 hotový - vrtulník ve 3D (`docs/visual-concept.md`). Modely dělají Blender skripty v gitu
  (`5_remake/game/Blender/`, `blender -b`), v `Assets.json` nový druh `generated` (skript, jeho vstupy = sady textur
  manifestu, výstupy; nic se nestahuje): `fetch-assets.ps1` je vyrobí do `assets/3d/generated/` a znovu, kdykoli je
  některý skript v `Blender/` novější než výstup, `build.ps1` je naimportuje. `copter.py` (+ `copter_materials.py`,
  `copter_layout.py`, sdílené `ugh_kit.py`: procedurální textury v numpy, PBR materiály, trubky, balvany, export glTF):
  pravěký šlapací vrtulník podle spritu - klec z bambusu svázaného provazem, podlaha z kmenů (kůra `palm_bark`), proutí
  s průhlednými dírami vzadu a po stranách, pařezy s koženými polštáři, řídítka z kosti, klika s kamennými pedály
  (`rock_face_03`), kožené prapory a kly; rotor z kostí s listy na kamenném náboji (hráč 2 keřovitější). Barvy týmu jsou
  barvy kůže (oranžová, tyrkysová). Tělo vyplňuje `COPTER_BODY_*` a do hloubky zůstává v ±45 jednotkách (za čelem skály
  -60 a bublinami). `caveman.py` (+ `caveman_rig.py`, `caveman_actions.py`): zavalitý kreslený jeskynní muž (1,15 m)
  z metaballů, leopardí kožešina, vlasy krátké / dlouhé a vousy jako vlastní sloty materiálů, kostra 18 kostí s vahami
  podle vzdálenosti ke kostem a akce `idle`, `sit`, `pedal`, `hang` z vlastního řešiče pózy (míření kostí, dvoukostrové
  IK). Import přes Interchange dal rovnou SkeletalMesh + Skeleton + AnimSequence na akci (`caveman<akce>`); kopie
  glTF materiálů v `_Masters` teď povolují i skeletal mesh. `stone_passenger.py`: visící cestující originálu je kámen
  s očima (`standingPassenger`, vzhled 4) - visí v provazové smyčce pod podlahou. UE: `AUghCopters` (místo kvádrů
  z `AUghFigures`; bez modelů dál plastelína, ověřeno), `FUghCaveman` (vzhled = viditelné sekce a barvy faktorů glTF,
  akce hrát / držet), `FUghRotorSpin` (rotor se točí plynule tak rychle, jak se mění `rotor_sprite`, klika 3x pomaleji,
  pilot šlape přesně s klikou), `UghBetween` (interpolace a skok bez interpolace pro všechny postavy),
  `UghCopterModel.h`. Cestující v kabině sedí za pilotem vpravo výš (vzhledy 1 mladík, 2 žena s dlouhými vlasy,
  3 stařec), kámen se houpe proti směru letu. Autopilot neveze nikoho, proto `shot.ps1 -Cargo <vzhled> [-Hanging]
  [-CloseUp]` (jen obraz, logika beze změny; zavřený záběr na vrtulníky). Testy `Ugh.Copter.Spin`, `Ugh.Copter.Model`
  (tělo v rozměrech těla logiky a v desce, kámen velký jako sprite). `levels.ps1` (27,5 min) a archy bez vad,
  prohlédnuté i snímky (tým, bouřka, vrtulník na vodě, kabina s cestujícími, visící kámen). CTest logiky,
  `6_verification` (163) a 169 testů v UE zelené. Pro krok 17: postavu cestujících brát z `FUghCaveman` (nové akce
  do `caveman_actions.ACTIONS` a `EUghCaveAction`), počátky akcí: `idle` na zemi mezi chodidly, `sit`/`pedal` sedák,
  `hang` úchop; čelem k +Y (ke kameře). Čeká na Jana: prohlédnout vrtulník v okně (`play.ps1`). Další: **krok 17**.
- 2026-10-04: krok 17 hotový - cestující a nepřátelé ve 3D (`docs/visual-concept.md`). C API (logika beze změny, 161
  replayů zelených): `ugh_logic_entity.look` (vzhled cestujícího jako `cargo_look`) a `.stunned` (nepřítel vyřízený
  cestujícím: omráčený walker a foukač, padající letec - jejich sprite je snímek letu / foukání), `ugh_logic_get_sprite`
  (jméno spritu v datech: animace druhu a snímek, nebo vlastní sprite pravidel či druh bonusu); `api/LevelView` (pohled
  levelu přes Visitory), testy C API. Blender: sdílené `ugh_rig.py` (kostra z tabulky, váhy podle vzdálenosti, akce
  z póz s dvoukostrovým IK, i měřítko kostí), `ugh_blobs.py` (metaballs), `creature_kit.py` (šupinatá kůže, oči, rohy,
  hvězdičky omráčení); jeskynní muž má navíc `walk`, `wave`, `tread`, `swim`, `fall`; nové modely `pterodactyl.py`
  (`fly`: dvě mávnutí a plachtění, `fall`), `triceratops.py` (`walk`, `watch`, `charge`, `recover`, `stunned`),
  `blower.py` (bradavičnatá šelma s chobotem: `blow` nádech 3/10 a výdech, `stunned`), `fruit_tree.py` (strom s tváří,
  `sway` s mrknutím v posledním snímku, `shaken`), `bonus_items.py` (9 plodů a kamenná tabulka X, síť pojmenovaná podle
  druhu bonusu) - v `Assets.json`. UE: `FUghFigureActions` (tabulka pravidel jméno spritu -> model, akce, směr, zda
  akce sleduje snímky animace spritu; z entity omráčení a pád ve vodě), `FUghFigureModels` (komponenta na entitu,
  `FUghRig` = obecný riggovaný model, `FUghCaveman` nad ním), `UghFigurePlace` (počátky modelů podle spritu: nohy na
  spodku, ve vodě hladina 4 px pod vrškem, letec uprostřed naklopený 35° zády ke kameře a 1,3× větší, ze dveří
  zezadu), `FUghFrameClock` (akce plynule, ale v rámci snímku spritu). Bez modelů dál plastelína. `shot.ps1 -Frame
  <x>,<y>,<w>,<h>` (záběr části obrazovky). Testy `Ugh.Figures.Actions` (každý z 269 spritů entit dat má pravidlo
  a akci svého modelu), `.Clock`, `.Models`.
  `levels.ps1` (27 min) a archy bez vad, prohlédnuté i snímky (`-Frame`: chodící a netrpělivě mávající cestující,
  letec, triceratops, foukač; bouřka, tým). Screech letce originál nekreslí (letec je při něm skrytý), proto akce nemá;
  bonusové předměty autopilot nevyvolá, ověřené testem a náhledem v Blenderu. CTest logiky, `6_verification` (163)
  a 172 testů v UE zelené. Pro krok 18: postavy stojí v desce hry (-20 .. 20), triceratops a foukač sahají do hloubky
  asi ±60 a natáčejí se ke kameře, letec v letu až ~±170 (křídla); dekorace na římsách s nepřítelem nebo plošinou mají
  nechat volný prostor nad nimi (strom 2,6 m, triceratops 1,9 m) a za deskou; stromy s tváří jsou nepřátelé logiky,
  palmy dekorace. Čeká na Jana: prohlédnout postavy v pohybu v okně (`play.ps1`). Další: **krok 18**.
- 2026-10-04: krok 18 hotový - příroda a dekorace (`docs/visual-concept.md`). Pravěká džungle na římsách: louky
  (instancované trsy trávy `grass_medium_01/02` v řadách do hloubky, ostrůvky květin `flower_gazania`,
  `flower_empodium`, `periwinkle_plant`), palmy (až 4 nejvyšší, které se vejdou), keře, kapradiny, rostliny džungle
  (`anthurium_botany_01` s červenými květy, `calathea`), kameny, pařezy `tree_stump_01`, kosti a lebky, dva totemy,
  chýše, až 3 ohně a liány ze stropů a závěsy lián po zadní stěně jeskyně; v průměru ~2000 dekorací na level (nejméně
  286). Z Blenderu nové `bones.py`, `totem.py`, `hut.py` (+ `prop_shapes.py`) a `vines.py` (v `Assets.json`). Rozmístění
  (`UghDecorations::Plan` a `UghPlans`: `UghDecorations.cpp`, `UghPlants.cpp`, `UghVines.cpp`) deterministicky z masky
  a z pole skály (`FUghGround` nad `FUghRockField`: podlaha / strop / zadní stěna v hloubce dekorace, přední polovina
  krabice ve vzduchu), pravidla v `FUghPlacer`: nic na obrazovce blíž než 25 jednotek za rovinou hry, nic vyššího než
  5 px blíž než 80 (tělo vrtulníku 45, nepřátelé 70), nic vyššího než 16 px ani liána blíž než 180 (rotor 130, křídla
  letce 170), u plošin místo pro tělo (80) a rotor (180) přistávajícího vrtulníku; listnaté se smí prolínat. Test
  `Ugh.Scenery` pro 150 levelů obou režimů: bohatost (aspoň 150 dekorací, 6 druhů, 100 trsů, palma), determinismus,
  na skále a ve vzduchu, žádný průnik s chráněnými objemy (deska hry, dosah postav, rotorů a křídel, přistání na
  plošinách, cedule s čísly) a s ohněm. Oheň (`AUghCampfire`): Kenneyho kameny a polena v tmavé plastelíně, plamen ze tří
  zkřížených kartiček s novým materiálem `M_UghFire` (`Shaders/UghFlame.hlsl`: jazyky z šumu, jiskry, vítr), blikající
  Lumen světlo každého ohně, zhasne pod vodou. `FUghRockField::Build(Logic, Art)` plánuje krápníky sám, síť
  `FUghRockMesh::Build(Field)`. Import: průsvitné glTF materiály (BLEND: tráva, květiny) vyříznuté (Nanite průsvitnost
  nekreslí), kopie v `_Masters` povolují i instancované sítě (`ImportVersion` 5), generované materiály povolují Nanite.
  `AUghScenery`: komponenta ISM na síť, tráva a květiny bez stínů, maskované listy bez ray tracingu, bez modelů
  plastelína (ověřeno s přejmenovaným `Content/Imported`). `levels.ps1` vypíše medián fps: 40 (krok 17: 36; uprostřed
  běhu po zahřátí propad na 13-25, pak zpět 40+), jednotlivé snímky 49-53. `levels.ps1` (27 min) a archy bez vad,
  prohlédnuté i snímky. CTest logiky, `6_verification` (163) a 172 testů v UE zelené. Pro krok 19: voda zakrývá
  dekorace pod hladinou (ohně zhasínají podle `SetWater`), dekorace jsou za deskou 25-500 jednotek hluboko, závěsy lián
  na zadní stěně; ohně jsou 2-3 bodová světla se stíny (25 cd, dosah 900) - při změně nálady levelu (noc) je využít;
  nepoužité assety `island_tree_02`, `pachira_aquatica_01`, `dead_tree_trunk`. Další: **krok 19**.
- 2026-10-04: krok 18b hotový - fotorealistický svět z Epicova vzorku Electric Dreams (`docs/visual-concept.md`).
  Vzorek (`..\Unreal Projects\ElectricDreamsEnv`, UE 5.8) se jen čte; inventura z miniatur v hlavičkách `.uasset`
  (tabulka s velikostmi ve `visual-concept.md`). `electric-dreams.ps1` (commandlet `UghCopyElectricDreams`: obsah vzorku
  připojený jako `/UghSample/`, tvrdé závislosti z asset registry, kopie souborů balíčků beze změny, nepotřebné smaže,
  načte vše) zkopíruje 78 assetů z `UghElectricDreams.h` s 257 balíčky (3,1 GB) do `Content/External/ElectricDreams`
  (gitignore); `[CoreRedirects]` v `DefaultEngine.ini` posílají odkazy vzorku `/Game/Megascans|MSPresets|Custom|
  SmartAssets|PhysicalMaterials/` do kopie (commandlet chybějící přesměrování ohlásí). Materiály vzorku jsou Substrate
  a textury virtuální (bez toho černé): projekt má `r.Substrate=True` a `r.VirtualTextures=False`. Zadní stěnu jeskyně
  pokrývají velké naskenované pískovcové útesy (`HugeSandstoneCliff`, `MassiveSandstoneCliff`; `UghRockDressing`,
  `AUghCliffDressing`: 40-65 px, čelem ke kameře, překrývají se, nikdy blíž než 250 jednotek ani než prostředek dekorace
  před nimi), ze stropů visí kořeny (`SM_Roots_01..05`). Dekorace kroku 18 mají modely vzorku (palmy `AlexandraPalm`,
  `FanPalm` ..., rostliny s velkými listy `Taro`, `BirdOfParadise`, kapradiny, keře, tráva `KikuyuGrass`, květiny,
  kameny, pařezy, liány a závěsy břečťanu `Ivy`); kosti, totemy, chýše a ohně zůstaly. Čelo skály (`M_UghCliff`):
  dlaždicové povrchy vzorku (`T_Rock_03`, `T_Rock_01`, `MossyGrass`, `NordicMoss`, `JungleGround`), reliéf podle
  `HeightMask`, velké skvrny proti opakování, kresba změkčená na 8 px a tónující jen trochu (cihly zmizely), teplá skála
  do pískovce. Zamítnuto (vyzkoušeno): naskenované textury útesů (zjevné opakování), balvany a kořeny vsazené do čela
  v rovině hry (vypadaly jako nalepené oblázky - pevné oblasti masky jsou na ně tenké). Světlo a post: EV100 1,5, bloom,
  teplé vyvážení bílé, sytost a kontrast, vinětace. Test `Ugh.Dressing` (150 levelů: deterministicky, za dosahem postav
  a za dekoracemi, aspoň 10 útesů, nejméně 23); `Ugh.Rock` a `Ugh.Scenery` beze změny zelené. Bez kopie vzorku hra ukáže
  CC0 assety (ověřeno shotem s odsunutou kopií). `levels.ps1` (26,8 min) a archy bez vad, prohlédnuté i snímky (1p-01,
  1p-03, 1p-43 bouřka se stoupající vodou, team-21, detaily). Výkon (Radeon 890M): jednotlivé snímky 42-46 fps,
  `levels.ps1` medián 43 (krok 18: 40), nejpomalejší 27 - škálování kvality netřeba. CTest logiky, `6_verification`
  (163) a 173 testů v UE zelené. Opraven i rozbitý první řádek `5_remake/game/README.md`. Čeká na Jana: posoudit vzhled
  v okně (`play.ps1`), hlavně čelo skály (vápenec místo cihel kresby). Pro krok s MetaHumany: projekt už běží se
  Substrate (materiály MetaHumanů 5.8 s ním počítají) a bez virtuálních textur (textury MetaHumanů se postaví jako běžné); postavy z
  `..\Unreal Projects\MetaHumans` přenést stejnou cestou (commandlet + `[CoreRedirects]`), ale pozor na pluginy
  MetaHuman (závislosti mimo `/Game` commandlet jen ohlásí). Se Substrate je nahraná PSO cache stará (`pso.ps1` znovu
  před vydáním); balení s `Content/External` nevyzkoušené. Další: **krok 19**.
- 2026-10-04: krok 18c hotový - fotorealistická čelní skála (`docs/visual-concept.md`). Čelo skály bylo plochý světlý
  „vápenec“; teď je to vrstevnatý naskenovaný pískovec ladící s útesy vzadu. Geometrie (`FUghRockField::Front`, jen před
  deskou hry): reliéf 0,8-4 px - zvlněné vrstvy pískovce asi 9 px vysoké, každá u paty víc venku se zářezem pod sebou
  (stínové linky), jinde výrazné, jinde skoro žádné; skoro svislé pukliny, široké boule, zrno. Barvy vrcholů
  (`FUghRockMesh`): modrá = blízkost pod horní hranou masky (2-8 px, kde je nad ní vzduch: tráva a mech přes hranu),
  alfa = velké skvrny (Perlin po metrech). `M_UghCliff` (`UghCliff.hlsl`): vrstva skály je naskenovaný `BeachCliff`
  vzorku (v 18b odmítnutý pro opakování) ve dvou měřítkách (5 a 7 m) s posunem, mezi nimi rozhodují velké skvrny
  a reliéf - dlaždice se neopakují; `T_Rock_03` šedé skvrny v pískovci, zadní stěna a jemná normála na celé skále;
  tráva nahoře a pruh přes horní hrany, pod ním mech, hlína na svazích a ve škvírách, škvíry a prohlubně tmavší (i AO),
  skvrny světlejší / tmavší a teplejší / šedší, pískovec ztlumený do okrové, u vody a pod ní mokrá tmavší lesklá skála
  (nový parametr `WaterLevel` z `AUghBackground::SetWater`; alfa barvy vrcholů do kódu přes `AppendVector`).
  `electric-dreams.ps1` znovu (78 assetů, 257 balíčků, 3,18 GB; `T_Rock_01` pryč, `BeachCliff` přibyl). Vyzkoušeno
  a zahozeno: skála víc do červena (BeachCliff bez ztlumení), pravidelné vrstvy s pevnou výškou (vypadaly jako
  naskládané desky). Snímky před a po (1p-01, 1p-03, 1p-43, team-21: `Saved/Shots/Before18c/pair-*.png`) a archy
  `levels.ps1` (27,2 min) bez vad; bez kopie vzorku (odsunutá `Content/External`) stejný materiál s CC0 sadami, shot
  prošel. Výkon: `levels.ps1` medián 41 fps (18b: 43), nejpomalejší 25. CTest logiky, `6_verification` (163) a 173
  testů v UE zelené (`Ugh.Rock` beze změny). Volitelné vsazení naskenovaných balvanů za hrany čela nezkoušeno (v 18b
  vypadaly jako nalepené). Čeká na Jana: posoudit čelo v okně (`play.ps1`), hlavně odstín (okrová / šedohnědá).
  Další: **krok 19**.
- 2026-10-04: krok 18d hotový - fotorealistické postavy z MetaHumanů (`docs/visual-concept.md`). Vzorek MetaHumans
  (UE 5.7) má jen dvě hotové postavy (Ada, Taro) bez stařce, proto zdrojem je MetaHuman Creator v editoru (plugin
  `MetaHumanCharacter` + `PythonScriptPlugin` v `UghGame.uproject`, volitelný obsah „Core Data“: 29 předloh): skript
  `Python/metahumans.py` (`metahumans.ps1`, editor bez okna - commandlet neumí zapéct textury grafem) zkopíruje
  předlohu, nasadí vlasy a vousy, vyžádá z Epicova cloudu obličejový rig a textury kůže (přihlášení Jana fungovalo bez
  dialogu) a sestaví `Optimized`/`Low` do `Content/External/MetaHumans/<jméno>` (gitignore, ~1,5 GB), barvy vlasů
  parametry materiálů. Pilot `Mateo` (dlouhé rozcuchané vlasy, strniště), vzhled 1 muž `Bruce` (plnovous), 2 žena
  `Celeste` (dlouhé zrzavé vlasy), 3 stařec `Walter` (bílé vlasy a vousy); vzhled 4 dál kámen. Oděv Creatoru (tričko,
  šortky) v leopardí kožešině jeskynního muže (slot `fur` z `caveman.glb`); holá hruď nejde (tělo je pod oděvem
  vyříznuté). Akce (`Python/metahuman_actions.py`, `metahuman_poses.py`, `ugh_math.py`) v proporcích každé postavy:
  `idle` a jeden krok chůze z klipů Creatoru (na místě), ostatní pózy dvoukostrovým IK (mávání, sezení, šlapání na
  pedály kliky a ruce na řídítkách podle `copter_layout.py`, visení, šlapání vody, plavání, pád), počátky jako
  u jeskynního muže. UE: `FUghMetaHuman` (komponenty blueprintu bez actora: tělo hraje akci, obličej a oděv leader
  pose, groomy na obličeji v detailu 5 = helma, bez fyziky - karty kvalita Low nevyrábí, vlákna drahá), `FUghCaveman`
  je teď „lidé podle vzhledu“ (holder vysoký jako jeskynní muž 115 cm, MetaHumani, jinak Blender jeskynní muž -
  ověřeno s odsunutou složkou), osoba se při změně vzhledu vyrobí znovu. `shot.ps1 -CloseUp -Frame` = výřez od rohu
  vrtulníku. Nový test `Ugh.Figures.People` (na zemi a v desce hry, na sedadle, nohy na pedálech, ruce na řídítkách
  a laně, hlava u hladiny). Snímky: kabina se všemi třemi cestujícími (`1p-01-cargoN-closeup`, staré v
  `Saved/Shots/Before18d`), chůze, stání a vycházení ze dveří (1p-03), pózy vody a mávání ověřené dočasným přepnutím
  akce cestujícího v kabině. `levels.ps1` (27,7 min) a archy bez vad, medián 40 fps (18c: 41), nejpomalejší 24. CTest
  logiky, `6_verification` (163) a 174 testů v UE zelené. Čeká na Jana: posoudit postavy v okně (`play.ps1`) -
  leopardí tunika vs. obyčejná kůže (barvy `HIDE`/`FUR` v `metahumans.py`), postavy jsou v kabině menší než kreslený
  jeskynní muž (výška 115 cm), vousy jen jako helma (karty by chtěly sestavení `Medium`); vzorek
  `..\Unreal Projects\MetaHumans` se nepoužil. Další: **krok 19**.
- 2026-10-05: krok 19 hotový - voda, déšť, obloha, světlo (`docs/visual-concept.md`). Voda je moře kolem útesu
  (`AUghWater`, `UghWater`; Jan chtěl místo tyrkysové desky fotorealistické moře): materiál Single Layer Water
  `M_UghWater` (`Shaders/UghWater.hlsl`, `UghMakeWeather.cpp`) - engine kreslí, co je pod hladinou, skrz tolik vody,
  kolik pohled projde (lom, pohlcování červené, rozptyl do modrozelena), odrazy útesu a oblohy Lumenem; hladina je
  plochá přesně ve `water_level` (mezi dvěma kroky interpolovaná, nový level či pokus bez interpolace), vlny jsou
  v normálách (příboj valící se k útesu, čeřiny po větru), kroužky kolem plavců a vrtulníku na vodě
  (`UghWater::Rings`), kroužky kapek v dešti, pěna u skály (dýchající pruh rozbitý šumem), kaustiky na tom, co leží
  pod hladinou (podle místa, kudy tam vniklo slunce), hlouběji tmavší a modřejší. Moře sahá od 50 m před rovinou hry
  (kamera je 64 m) přes zadní stěnu jeskyně a 200 m do stran (`UghWater::Front`, `Reach`), nikde šev; jeho řez je
  vidět, jen když voda přeroste kameru. Vyzkoušeno a zahozeno: řez vody těsně a 2-20 px před čelem skály (vitrína:
  dvoubarevný pruh u hladiny). Déšť (`AUghRain`, `UghRain`): 6000 čar v krabici mezi kamerou a útesem, které posouvá
  materiál `M_UghRain` (`UghRain.hlsl`: síť drobných čtverců, GPU, nic za snímek), šplouchnutí na římsách a na moři
  (`M_UghSplash`, `UghSplash.hlsl`), kapky logiky (originál je kreslí, s ničím nekolidují) jako světlé čáry na
  kartičkách (`M_UghRaindrop`) místo plastelíny z `AUghFigures`; vše padá jako kapky logiky, stejně daleko po větru
  jako dolů. Niagara ne: její systémy jsou binární assety z editoru, ne kód; stejný výsledek dává materiál. Nálada
  levelu (`UghMood`, `AUghStage::SetMood`): klidné levely podle čísla v pořadí režimu (den, den, večer, večer,
  soumrak, noc), větrné bouřka; slunce (v noci měsíc) zepředu zleva, k večeru níž a tepleji, obloha HDRI nálady (nové
  `sky_qwantani_dusk`, `sky_qwantani_night`, `sky_kloofendal_overcast` v `Assets.json`), sky light, mlha, pevná
  expozice každé nálady (noc temná, hlavní světlo ohně, postavy čitelné); v bouřce mlžný opar nad hladinou (druhá
  mlha, stoupá s vodou) a rostliny vzorku Electric Dreams se klátí silněji a po větru (`MPC_GlobalFoliageActor`).
  Objemová mlha dřív končila 60 m od kamery, před útesem (stála výkon a nic nedělala); teď sahá k útesu v hrubší
  mřížce, paprsky slunce v jeskyni jsou jen jemné. Opraveno: `DeleteAllMaterialExpressions` enginu maže z pole,
  které prochází (v generovaných materiálech zůstávala polovina uzlů) - commandlet maže po kopii. Testy
  `Ugh.Water.Level` (level 3 se stoupající vodou: hladina `AUghWater` v každém snímku 20 s hry přesně na
  interpolované `water_level`, moře přes okraje obrazovky a ke kameře), `Ugh.Rain.Wind` (levely 43 a 64, vítr vlevo
  a vpravo: kapky logiky, kartičky i čáry deště jdou stejným směrem) a `Ugh.Mood`. Snímky 1p-03, 1p-08 (stoupající
  voda po 30 s), 1p-43 (bouřka se stoupající vodou), team-21, noc 1p-06 / 1p-36, soumrak 1p-05 / 1p-11, detaily vody
  (`-Frame`). `levels.ps1` (28 min) a archy bez vad. Výkon (Radeon 890M): jednotlivé snímky 37-44 fps, ale v dlouhém
  `levels.ps1` přejde GPU po pár minutách do úsporného stavu (~25 místo ~40 fps) - medián 24, proto hrubší mřížka
  objemové mlhy a odrazy moře v polovičním rozlišení (`DefaultEngine.ini`, vypadá stejně): medián 26, nejpomalejší
  21. CTest logiky, `6_verification` (163) a 177 testů v UE zelené. Čeká na Jana: posoudit moře, déšť a nálady
  v okně (`play.ps1`) - hlavně jak tmavá smí být noc a jak oranžový soumrak (`UghMood.cpp`). Pro krok 19e: moře je
  jeden box `UghWater::Box` (krychle), pro průlet stačí zvětšit `Reach` a `Front` (řez před kamerou pak schovat), vlny
  jsou jen v normálách (hladina plochá kvůli testu výšky). Další: **krok 19b**.
- 2026-10-05: krok 19b hotový - lidé: listy, vlasy, sezení (`docs/visual-concept.md`). MetaHumani znovu
  (`metahumans.ps1 -All`): bez oděvu (slot `Outfits` prázdný - tělo celé, pod listy spodní prádlo textur kůže),
  kvalita `Medium` (karty vlasů jen v detailu 3, hra drží `GroomLOD` 3: skutečné vlasy, vousy i obočí, ne helmy).
  Listy (`FUghLeaves`, `UghLeaves.cpp`): bederní rouška z listů tara Electric Dreams (`UghElectricDreams::LeafMaterial`,
  výřezy listů v atlasu `LeafPictures`), každý list síť ovinutá kolem těla podle fyzikálního assetu postavy, postavená
  za běhu v prostoru své kosti: vpředu a po stranách na stehnech (jdou s nohama, při sezení leží na klíně), vzadu
  a krátký list před rozkrokem na pánvi, žena navíc pás přes prsa (`spine_05`); bez vzorku zelená plastelína.
  Velikost: `UghFigurePlace::PersonHeight` 145 cm (chodící postava s vlasy vyplní 14 px spritu, o ~5 % nižší než
  vestoje), stejná při chůzi, čekání, plavání i v kabině (dřív 115 cm v kabině, na zemi natažená podle spritu);
  jeskynní muž z Blenderu škálovaný stejně. Kabina (`copter_layout.py`, `UghCopterModel.h`, `copter.py`
  a `caveman_actions.py` znovu): pilot sedí natočený o 50° doleva a šlape, cestující o 50° doprava (kamera vidí sezení
  z boku), klika a řídítka v rámci pilota (`PedalAxle`, `Grip`), pedály dál a níž, sedadlo pilota o 8 cm dozadu (tělo
  vrtulníku dál v ±45). Veze-li vrtulník kámen (vzhled 4) v kabině, sedí na sedadle zmenšený na 0,32 a dívá se do
  kamery (`SeatedStone`). Opraveno: akce MetaHumanů engine „retargetoval“ ze skeletu (sedící a mávající se vznášeli
  ~7 cm) - teď `retarget_source_asset` = tělo, a test `Ugh.Figures.People` počítá pózy jako hra (`GetAnimationPose`
  na síti), navíc kontroluje listy (boky od pasu po stehna, u ženy hruď); `Ugh.Copter.Model` kámen na sedadle.
  Výkon: karty vlasů stály ~1 ms GPU a `levels.ps1` spadl na medián 23 (nejpomalejší 18); lidé teď nejsou ve scéně
  ray tracingu a `DefaultEngine.ini` šetří, co není vidět (odrazy Lumenu poloviční, stíny slunce hrubší, objem
  osvětlení průsvitných 32): `levels.ps1` (27,9 min) medián 25, nejpomalejší 16; archy bez vad, prohlédnuté i detaily
  (`shot.ps1 -Cargo 1/2/3/4 -CloseUp [-Frame 6,4,22,14]`, 1p-03 `-Frame` s chodící ženou a stařcem, team-21).
  CTest logiky, `6_verification` (163) a 177 testů v UE zelené. Čeká na Jana: posoudit listy a vlasy v okně
  (`play.ps1`) - hlavně zda stačí pás listů u ženy (z boku je vidět šedá podprsenka textury); fps je na hraně
  (medián přesně 25), další rezervu by dalo `r.Nanite.MaxPixelsPerEdge=2` nebo hrubší Lumen GI. Bez MetaHumanů
  (jeskynní muž) nevyzkoušeno znovu shotem. Další: **krok 19c**.
- 2026-10-05: krok 19c hotový - skály podle reference: šedý krasový vápenec, vchody do jeskyní
  (`docs/visual-concept.md`). Čelo skály: vrstva skály je Poly Haven `marble_cliff_03` (rozpukané bloky a římsy,
  nové v `Assets.json`, `cliff_side` vypadl) převedená v `UghCliff.hlsl` do světle šedého vápence s vyšším
  kontrastem a tmavými stékanými pruhy, šedý kámen je `mossy_rock` (lišejník), mech ve škvírách, na malých římsách
  a ve skvrnách, tráva jen na plochách nahoru a přes hrany, suchá skála matná (drsnost aspoň 0,75), mokrá jen do metru
  nad vodou, kresba už netónuje barvu; tráva, mech a hlína dál ze vzorku Electric Dreams, každá vrstva zvlášť
  (`AUghBackground`; `BeachCliff` a `T_Rock_03` z kopie vypadly). Reliéf čela (`UghRockNoise`): rozpukané bloky
  s rovnými nakloněnými plochami místo vrstev pískovce, jen před deskou hry. Útesy vzadu: šedé
  `HugeNordicCoastalCliff_01/_02`, `MassiveNordicCoastalCliff_01` položené na bok (lavice), přebarvené novým
  levným materiálem `M_UghScan` (jejich textury, 30 % sytosti, šedý tón, mech nahoře) - A/B snímky team-21 stejné fps
  jako s Megascans masterem vzorku; `MossyRockFace_03` vyzkoušen a zahozen. Vchody do jeskyní (`FUghCavePortal`,
  `UghCavePortals`, `FUghLevelArt::Doors`): každé dveře kresby (dřevěný rám 60/61/80/81 i ústí 62/63/82/83, 377
  v 81 levelech) jsou v poli skály oblouk rozpukaných bloků 18 px za rovinou hry (za dosahem rotoru a křídel), který
  splývá se zadní stěnou, otvor 22 x 22 px, chodba 72 px hluboko stočená ke kraji, stěny chodby ve vrcholových barvách
  zavřené (tma); cestující vychází z tmy 21 px hluboko (`DoorDepth`). V chodbě nic jiného: dekorace jen pokryv před
  obloukem (`FUghPlacer`), útesy a kořeny se zmenší nebo vypadnou, krápníky a kameny před vchodem ne. Nový test
  `Ugh.Portals`; `Ugh.Rock` teď s vchody (nejvýš 0,12 px), `Ugh.Scenery` a `Ugh.Dressing` hlídají chodby
  (nejméně 21 útesů, 187 dekorací). Snímky reference / před / po v `Saved/Shots/Before19c/pair-*.png` (1p-01, 1p-03,
  1p-43, team-21), archy `levels.ps1` (33,5 min) bez vad (večer a soumrak barví vápenec do okrova), bez
  `Content/External` stejná skála s CC0 trávou a mechem, shot prošel. CTest logiky, `6_verification` (163) a 178
  testů v UE zelené. Výkon: `levels.ps1` medián 19 fps, nejpomalejší 8 - ale notebook je teď (ráno po uspání, Jan
  v práci) pomalejší celkově: jednotlivý snímek team-21 24-29 fps (v noci 18b 42-46), prvních 45 levelů běhu 31-35,
  pak propad na ~18 (úsporný stav GPU jako v kroku 19); A/B na tomto stavu: vchody ani materiál útesů fps nemění
  (rozdíly v šumu ±3). Čeká na Jana: posoudit vápenec a vchody v okně (`play.ps1`) a pustit `levels.ps1` doma
  v noci znovu kvůli fps (rezerva `r.Nanite.MaxPixelsPerEdge=2` dala +2 fps); dřevěné rámy dveří kresby zatím jen
  skalní oblouk. Další: **krok 19d**.
- 2026-10-05: krok 19d hotový - cedule s čísly a bubliny (`docs/visual-concept.md`). Cedule (`UghPadSigns`,
  `AUghSigns`, `Blender/signs.py`, nový asset `signs` a textura Poly Haven `rough_wood` v `Assets.json`): zvětralé
  prkno přibité na křivém kůlu, číslo vyřezané čárkami jako sprity originálu 85-90 (I až IIII, pět = čtyři přeškrtnuté
  pátou, prázdné prkno), zářezy světlé čerstvé dřevo - čitelné i v noci. Každá plošina právě jednu: kde má kresba
  originálu ceduli na plošině, tam (716 z 806 v 150 levelech), jinak co nejblíž středu mimo vchody do jeskyní a jiné
  cedule (47 plošin v 81 mapách, kam nikdo nejezdí). Značky z čísla plošiny logiky (`ugh_logic_get_pad`); kresba se
  s ním shoduje až na přehlédnutí originálu (level_id 31: II i na plošině 1) a druhou ceduli na téže plošině (level_id
  74). Plošina čísla 6 (nepojmenovaná) prázdné prkno jako v originálu. Cedule stojí 88 jednotek za rovinou hry (za
  dosahem těl vrtulníků a nepřátel), dekorace před ně nesmí (`FUghPlacer`, `Ugh.Scenery`); bez modelů karta se spritem.
  Bubliny (`UghBubbles`): ostrý obrázek kreslený za běhu ze vzdáleností (bílá bublina, prkénko s čárkami cílové
  plošiny, od šesté prázdné, nebo otazník), co ukazují, říká logika jménem spritu (`ugh_logic_get_sprite` teď jmenuje
  `destinationBubble` s indexem plošiny a `impatientBubble`; test logiky). Zobáček: originál má jen sprity se
  zobáčkem vlevo dole a kreslí bublinu vždy 11 px vpravo a 13 px nad cestujícím (`Frame.kt`) - zobáček míří na hlavu
  cestujícího, ne k vrtulníku. Dva zobáčky byly chyba karty: krychle s oboustranným `M_UghSprite` ukazovala zezadu
  zrcadlený obrázek přes průhledné místo vpředu; materiál je teď jednostranný. Bublina se kreslí na místě originálu,
  jen kdyby vyjela vpravo z obrazovky, zrcadlí se nalevo se zobáčkem vpravo. `shot.ps1 -Bubbles` dá bublinu každému
  cestujícímu (detaily 1p-01 `-Frame`). Nová pravidla snímků (Jan): `levels.ps1 -Quick` (12 levelů 1p 1, 3, 6, 8, 12,
  23, 36, 43, 62, team 1, 21, 54 na archu `levels-quick.png`, 2,7 min) po každém kroku, celý běh jen jednou denně
  v noci nebo před milníkem. Testy `Ugh.Signs` (každá plošina všech 150 levelů právě jednu ceduli se správným číslem,
  ověřeno proti spritu kresby, na skále, nad plošinou, bez překryvu) a `Ugh.Bubbles` (obsah všech bublin dat, strana
  zobáčku pro každou polohu cestujícího, špička na hlavě, obrázek s jedním zobáčkem). CTest logiky, `6_verification`
  (163) a 180 testů v UE zelené. Výkon (A/B ve stejném běhu, rychlá sada): před krokem medián 34 fps (nejpomalejší
  31), po něm 35 (29). Čeká na Jana: posoudit cedule a bubliny v okně (`play.ps1`); jestli má zobáček přece mířit
  k vrtulníku (originál to nedělá), stačí změnit `UghBubbles::Place`. Celý `levels.ps1` tento krok nespuštěn (nové
  pravidlo). Další: **krok 19e**.
- 2026-10-05: krok 19e hotový - úvod levelu: let nad mořem ke kameni (`docs/visual-concept.md`). Celý dnešní útes je
  vytesaný do čela velkého samostatného kamene v otevřeném moři (`FUghStackField`, `AUghSeaStack`): krasová věž asi
  108 m široká, 52 m hluboká, 90 m nad hladinou, klenutý vrchol s džunglí ze vzorku Electric Dreams (palmy, keře,
  rostliny, kapradiny, kameny; keře na římsách, břečťan z horních hran stěn; asi 4000 kusů přes `AUghScenery`), boky
  s boulemi, žlábky od deště, vrstvami vápence s římsami a převisy, rozpukanými bloky a zářezem vln, pata se pod
  vodou rozšiřuje; čelo kolem skály levelu rovné (rám 1 m před rovinou hry, dál vystupuje o 2,2 m: level sedí ve
  výklenku). Skála levelu sedí v dutině kamene: otevřený okraj její mřížky i neviditelný rubáš stínů
  (`AUghBackground::ShroudBoxes`) jsou v kameni. Pole na mřížce 0,6 m a surface nets (`UghSurfaceNets` je teď šablona
  pro obě mřížky, `FUghRockMesh::Build` z bodů a čtyřúhelníků), materiál čela `M_UghCliff` (vrstvy skály 2,5krát
  větší, z dálky se neopakují); kámen se vyrobí při prvním letu (1-1,5 s, 164 tisíc vrcholů, 331 tisíc trojúhelníků)
  a mimo let je schovaný a odregistrovaný ze scény (stíny, ray tracing, Lumen: hře nic nestojí). Let (`FUghIntro`,
  kód místo Sequenceru): při prvním popisku levelu (ne po havárii) 8 snímků v černé (render se usadí), 0,3 s
  rozsvícení, 4,5 s z 260 m před kamerou hry 2,6 m nad vlnami po křivce Catmull-Rom s kličkou, polovinu času asi
  90 m/s, pak brzdí do zastavení; náklon do zatáček (nejvýš 10°), houpání výšky, objektiv 72° -> 30°, pohled po
  dráze a na level, menší rozmazání pohybem, venku expozice o 0,8 EV tmavší, ke konci zpět. Konec je přesně kamera
  hry (`AUghStage::Fit` + `SetCamera`, `FUghCameraPose`; `ScreenMargin` beze změny). Klávesa zrychlí zbytek na 0,6 s
  (vždy před začátkem hry: zhasnutí popisku a černá před hrou trvají 73 snímků logiky), jde do logiky jako dřív;
  logika a její časování beze změny. Během letu, popisku, černé před hrou a roztmívání je scéna vidět (dřív černá),
  popisek má stín; po havárii černý popisek jako dřív. `-UghNoIntro` nebo `FUghIntro::bFlies` let vypne. Moře
  během letu otevřené 4 km kolem (`UghWater::Box(…, true)`), v `UghWater.hlsl` daleko od kamene (od 25 m; kamera hry
  vidí jen 15 m před útes) vyšší vlny, bílé hřebínky, drsnější hladina, malé vlny s dálkou mizí, při pohledu skoro
  vodorovně klidnější a voda bez dna už není černá. Obloha nálady: kopule 10 km, mlha jen do 5 km
  (`FogCutoffDistance`), kamera ji vidí jasně jako světlo, které dává (`FUghMood::SkySeen`, v noci tmavá; `UghSky.hlsl`
  pozná zachycení sky lightem a to nechává beze změny) - světlo hry se nemění. `shot.ps1 -Intro <s>`
  (`-UghShotIntro`) uloží let v čase; prohlédnuté 1p-01 a 1p-03 (0,4 / 1,2 / 2,4 / 3,5 / 4,5 s: konec je snímek hry),
  1p-05 soumrak, 1p-06 noc (z otvoru svítí ohně), 1p-43 bouřka, team-21. Autopilot `shot.ps1` / `levels.ps1` let
  zrychlí první klávesou popisku. Nové testy `Ugh.Intro` (konec přesně kamera hry pro 16:9 a 16:10 a dvě výšky vody,
  start daleko a nízko, nad vodou, aspoň 2 m od kamene, nikdy za kamerou hry, plynulý, náklon, měkké zastavení,
  objektiv; hodiny: usazení, dlouhý snímek, zrychlení klávesou dřív než hra) a `Ugh.Stack` (okraj mřížky skály
  a rubáš v kameni, z kamery hry nic z kamene ani džungle, rostliny na kameni, deterministické), `Ugh.Water.Level`
  i s otevřeným mořem. CTest logiky, `6_verification` (163) a 182 testů v UE zelené. `levels.ps1 -Quick` (2,8 min)
  a arch bez vad (snímky hry jako v 19d), medián 30 fps (nejpomalejší 13 při zahřátém GPU); A/B ve stejném sezení
  s `-UghNoIntro` také medián 30 (notebook teď pomalejší než při 19d, 35). Offscreen běží let jen 8-15 fps, Lumen odrazy
  moře potřebují pár snímků (v prvních desetinách sekundy je moře tmavší). Čeká na Jana: posoudit let v okně
  (`play.ps1`) - rychlost a délku (`UghIntro.cpp`: `Waypoints`, `Cruise`, `Duration`), tvar a velikost kamene
  (`UghStackField.cpp`), oblohu a expozici nálad venku (`SkySeen`, `Outdoors`); postavy a vrtulník se objeví až se
  začátkem hry (při popisku je logika nemá); má-li být v kameře hry kolem levelu vidět víc kamene, zvětšit
  `ScreenMargin` v `UghStage.cpp` (hra se zmenší; nad 1,15 je třeba posunout dutinu kamene). Pro krok 19f: kámen
  je mimo let schovaný, vodopád do moře je tedy jen ve skále levelu; voda během letu je `UghWater::Box(…, true)`.
  Další: **krok 19f**.
- 2026-10-05: krok 19f hotový - pramen, potok, můstek a vodopád a obloha s mořem při letu (`docs/visual-concept.md`).
  Kde je místo (`UghStreams::Plan`, deterministicky z masky a pole skály: římsa s aspoň 14 px vzduchu nad sebou, pod
  ní skála celou cestu až 3 px pod hladinu v 7 px potoka a 4 px kolem, aspoň 12 px vysoko, dál než 16 px od plošin,
  mimo přistání vrtulníku, cedule a vchody do jeskyní, zadní stěna a podlaha k ní; nejvyšší vodopád, jeden na level)
  vytéká z díry v zadní stěně potok, teče korytem 1,5 px hlubokým vytesaným do skály jen před deskou hry a za ní
  (`FUghRockField::CarveChannels`; deska hry zůstává maska, `Ugh.Rock` i s korytem), pod můstkem z klád (vršky pod
  povrchem římsy, vzadu nízké zábradlí za postavami; `UghFalls::Bridge`, `rough_wood`) a zářezem v hraně čela padá
  jako vodopád před čelem do moře (vysouvá se ke kameře, s pádem se rozšiřuje). Voda je síť materiálu `M_UghFlow`
  (`UghFlow.hlsl`: průsvitná, vzor plyne s vodou, čeření a pěna v korytě, bílé pruhy, chuchvalce a mezery ve vodopádu,
  pod hladinou moře mizí - stoupající voda vodopád zkracuje), dole moře pění a vře a běží z něj vlny (`UghWater.hlsl`,
  `Fall0..1`) a stoupá mlha (`M_UghMist`: obláčky na kartičkách, které posouvá materiál jako déšť, nic za snímek).
  U pramene kameny a kapradiny (`UghPlans::AddSprings`), dekorace mimo koryto, most a pramen (`UghStreams::Rooms`),
  útesy zadní stěny nad pramenem ne. 38 ze 150 levelů (v rychlé sadě 1p-12, 1p-36, 1p-62); detaily 1p-36 a 1p-49
  (`-Frame`), celé 1p-12, 1p-29, 1p-30, 1p-48, 1p-49, 1p-51. Nový test `Ugh.Streams` (skála pod vodopádem až pod
  vodu, dál od plošin, cedulí a vchodů, klády pod povrchem a nad skálou, zábradlí za deskou a nízké, vodopád před
  čelem a stále blíž, pramen na stěně, voda nad korytem, deterministicky); `Ugh.Scenery`, `Ugh.Dressing` hlídají
  potoky. Společné sítě za běhu `UghMeshes` (i déšť). Oprava letu (review 19e): obloha byla jednolitě šedá - import
  dělá z HDRI krychli (`TextureCube`), materiál ji četl jako 2D a engine dosadil šedou výchozí texturu, kterou snímal
  i sky light; `M_UghSky` teď čte krychli: obloha nálady s mraky a září u obzoru a sky light snímá skutečnou oblohu
  (noci jsou ve hře o něco tmavší, jinak archy jako dřív). Otevřené moře daleko od kamene zrcadlí oblohu samo
  (`Mirror`: krychle oblohy podle odrazu vlny a Fresnela, pod ní hluboká modř; v bouřce méně), odrazy enginu tam
  slábnou, přibyly dlouhé vlny 34 a 21 m: modré moře s odrazy oblohy, čitelné vlny, bez černého zrnění (snímky
  `-Intro 0.4 / 2.4 / 4.5` 1p-01 den, 1p-03 večer, 1p-05 soumrak, 1p-06 noc, 1p-43 bouřka; staré v
  `Saved/Shots/Before19f`). CTest logiky, `6_verification` (163) a 183 testů v UE zelené. `levels.ps1 -Quick` a arch
  bez vad; fps A/B ve stejném sezení (notebook teď zahřátý a pomalý): před krokem medián 12, po něm 12 (ráno 31).
  Čeká na Jana: posoudit vodopády a let v okně (`play.ps1`) - šířku potoka (`UghStreams::Width` 7 px), kolik levelů
  (`MaxStreams`, `MinDrop`), mlhu u paty (`UghMakeFlow.cpp`). Další: **krok 19g**.
- 2026-10-05: krok 19g hotový - oheň a louče (`docs/visual-concept.md`). Niagara Fluids vyzkoušen (plugin zapnutý,
  hra bez okna pálila šablonu a scene capture snímal snímky): 3D šablony (`Grid3D_Gas_Fire` …) kreslí heterogenními
  objemy, které tu nekreslí nic ani v hlavním pohledu; 2D `Grid2D_Gas_SmokeFire` se zapéct dá, ale je to pevná scéna
  ohně rozfoukaného do strany bez parametrů - proto vlastní offline simulátor (zadání to připouští), plugin zase
  vypnutý. `FUghFireSim` (UghEditor): stabilní tekutina na posunuté mřížce 160 x 320, palivo hoří v teplo a saze,
  vztlak, vorticity confinement, stoupající šum dvou velikostí, MacCormack, tlak Gauss-Seidel; tři vrstvy se seedy
  vedle sebe (hloubka plamene). Commandlet `UghMakeFlames` (`build.ps1`, ~2 min) zapeče flipbooky 8 x 8 snímků 128 x 256
  (30 fps, smyčka prolnutím 16 snímků, ořez podle průměru, bílá = 98,5. percentil) do `T_UghFlameCampfire` /
  `T_UghFlameTorch` (bez streamování: jinak karta ukazovala jen nejmenší mipy - černo), náhled `Saved/Flames`. Materiály
  (`UghMakeFire.cpp`): `M_UghFlame` (flipbook na dvou zkřížených kartičkách, z boku mizí, vítr ho naklání; nahradil
  `M_UghFire`), `M_UghSparks` a `M_UghSmoke` (čtverečky posouvané GPU), `M_UghEmbers` (černé popraskané uhlí, popel,
  dýchající žár v prasklinách). Ohniště (`FUghHearths`): kruh kamenů `SmallStonesPack` vzorku Electric Dreams, spálené
  větve `OldTreeBranch`/`DryBranches` opřené jako stan, řeřavé uhlíky (9 assetů navíc v `UghElectricDreams.h`,
  `electric-dreams.ps1` znovu); bez vzorku Kenney. Louče (`AUghTorches`, nový Blender `torch.py` a asset `torch`):
  zaražené do skály, nakloněné ven, hlavice žhne, malý plamen, jiskry, dým; rozmístění `UghTorchPlan.cpp`
  (dekorace `Torch`: vedle vchodů do jeskyní na straně bez zatáčky chodby, pak na zadní stěně nad dlouhými římsami,
  nejvýš 4, za `SweepReach`, zaklíněné do stěny ve `FUghPlacer::Settle`, vlastní čísla náhody - ostatní dekorace
  beze změny; ohně mají box 15 px místo 12 kvůli vyššímu plameni). Světlo (`UghFireParts::FFlicker`): šum tří
  rychlostí mění jas, teplotu barvy (1850 K ± 350) a polohu (stíny a odlesky na stěnách tančí), měkké stíny, jas podle
  nálady (`FUghMood::FireLight` 1 / 1,05 / 1,15 / 1,3 / 1,1); oheň 9 cd, louč 0,8 cd; pod vodou zhasnou. Opraveno
  cestou: hloubka „ke kameře“ je +Y světa (louč se napoprvé zanořila do skály). `shot.ps1 -Look campfire|torch`
  (`-UghShotLook`) zabere první oheň / louč levelu. Snímky: `1p-01-campfire`, `1p-01-torch` (den), `1p-06-campfire`,
  `1p-06-torch` (noc), mihotání `1p-06-campfire-at2` vs. `-at2.1` (jiný tvar plamene i jas země). `Ugh.Scenery`
  hlídá i louče (na stěně, za dosahem rotorů, nejvýš 4; 470 loučí ve 148 levelech, u vchodu ve 148 ze 150 levelů se
  vchody). CTest logiky, `6_verification` (163) a 183 testů v UE zelené. `levels.ps1 -Quick` a arch bez vad. Výkon
  (rychlá sada A/B ve stejném sezení, notebook zahřátý): před krokem medián 15 (nejpomalejší 12), po něm 15 (5: první level po přestavbě materiálů ještě kompiluje shadery); dřív v sezení po kroku 23 fps. Čeká na Jana: posoudit oheň a louče v okně
  (`play.ps1`) - hlavně zda plamen ze simulace stačí (jinak oheň z Fabu), jas světel (`UghCampfire.cpp`,
  `UghTorches.cpp`, `FireLight` v `UghMood.cpp`); v noci horní louče silně prosvětlují strop. Další: **krok 20**.
- 2026-10-05: krok 19h hotový - nepřátelé a kámen fotorealisticky (`docs/visual-concept.md`). Nový druh assetu
  `local` v `Assets.json` (Janův T-rex z Fabu `trex_fab`, triceratops ze Sketchfabu `triceratops_sketchfab`, skeny
  Electric Dreams `electricdreams_scans`; licence Fab-Standard, Sketchfab-Download, Epic-Sample; nikdy se nestahuje
  ani necommituje): bez souborů je „absent“ a s ním i co se z něj dělá, hra pak ukáže modely kroku 17 (`fetch-assets.ps1`
  to vypíše, neselže; ověřeno odsunutím `fab/trex`). `electric-dreams.ps1` po kopii exportuje commandletem
  `UghExportElectricDreams` skeny `UghElectricDreams::ForBlender` (zdrojová síť, ne fallback Nanite, jako OBJ, textury
  materiálů jako PNG, parametry JSON) do `assets/3d/electricdreams`. Nové Blender skripty: `walker_triceratops.py`
  (import FBX opravený - kořenová kost má měřítko 0,2, které Blender dá síti, ale ne kostem, kostra pak vyšla 5× větší
  a kůže se trhala; síť ×5 kolem kořene, otočení, 3,1 m, 2× subdivize s ostrými hranami, kůže upečená Cycles ze
  ztmavlé textury + šupiny v prostoru modelu; akce walk/idle/run/attack1 = chůze/čekání/výpad/zotavení, omráčený =
  začátek die na břiše s kymácející se hlavou), `blower_trex.py` (low poly T-rex, kostra 27 kostí, automatické váhy,
  póza ležícího spícího zvířete jako klidová, zavřená tlama, oči zavřené domalováním textury, kost `nostrils`; akce
  `blow` hluboký nádech 3/10 a odfrknutí, `stunned` třepe hlavou; natočený jako ve hře zabere 3,1 m a nic není blíž
  kameře než 60 cm), `tree_hornbeam.py` (kmen z metaballs s naskenovanou kůrou habru upečenou ze tří os a tmavšími
  dutinami, vyřezaná tvář s vlhkýma očima a víčky z kůry, koruna ze dvou korun habru Sapling_03, plané jablko),
  `stone_boulder.py` (`MossyForestRock_02` z milionu na 40 tisíc trojúhelníků v elipsoidu kroku 16, oči pod víčky
  z kamene) a přepracovaný `pterodactyl.py` (pteranodon: zúžený zobák, hřeben, blána od prstu ke kotníkům, upečená
  kůže a blány se žilkami a vlákny); společné `ugh_bake.py` (pečení barvy, normály a drsnosti Cycles), `ugh_rig.build`
  umí automatické váhy, `creature_kit` vlhké oko a víčko, `ugh_kit.surface_uvs`. UE: `FUghFigureModels` bere nové
  modely, jinak staré (`UghAssets::Stone()` i pro vrtulník), blány letce dostanou `M_UghMembrane` (dvoustranný
  „foliage“, proti slunci teple prosvítají), T-rex při odfrknutí vyfoukne z nozder obláček prachu (`FUghSnort`,
  `M_UghPuff`, `UghPuff.hlsl`, `UghMakeFigures.cpp`; po zemi, kde leží hlava). Akce i jména akcí beze změny, tabulka
  `FUghFigureActions` platí; `Ugh.Figures.Models` měří zobrazené modely, kontroluje kosti `nostrils`/`head` T-rexe
  a blány letce. Snímky (detaily `-Frame` v den i noc): strom 1p-01, 1p-05, 1p-06; kámen 1p-01, 1p-12 a na sedadle
  (`-Cargo 4 -CloseUp` 1p-01, 1p-06); foukač 1p-04, 1p-19 (odfrknutí s prachem), 1p-18 noc; walker 1p-05, 1p-37,
  1p-18 noc; letec 1p-02, 1p-12 noc. CTest logiky, `6_verification` (163) a 183 testů v UE zelené. `levels.ps1
  -Quick` a arch bez vad. Výkon (rychlá sada A/B ve stejném sezení, nové importy odsunuté / vrácené, dvakrát): bez
  nových modelů medián 25 a 22, s nimi 23 a 25 - rozdíl v šumu. Čeká na Jana: posoudit nepřátele v okně
  (`play.ps1`) - hlavně tvář stromu (dosud trochu kreslená), barvu kůže triceratopsu (`walker_triceratops.py`
  `skin_graph`), sílu prachu z nozder (`UghMakeFigures.cpp`), zda T-rexovi stačí zavřená tlama (v koutku jsou vidět
  zuby); licence triceratopse ze Sketchfabu neznám (zapsána jen jako „stažený Janem“). Další: **krok 20**.
- 2026-10-05: krok 20 hotový - efekty událostí (`docs/visual-concept.md`, tabulka událost -> efekt v
  `5_remake/game/README.md` „The events seen“). Každá událost logiky má efekt (`FUghEffectPlayer::Cues`): popisek
  příboj u kamene (vidět z letu), havárie výbuch (ohnivé jazyky, kouř, třísky, jiskry, záblesk; do vody i šplouchnutí),
  dokončený level oslava nad vrtulníky (okvětní lístky, třpyt), nástup a puštění cestujícího prach, zaplacení mušlové
  peníze s třpytem a stoupajícími body (HUD), rychlé doručení a sebraný bonus třpyt (barva podle druhu bonusu,
  záblesk), cestující ve vodě šplouchnutí (kapky, tříšť, kroužky), křik letce peří, mávání proud vzduchu s prachem
  a chmýřím pod křídly od startu do stopu (smyčka jako zvuk), foukač poryv (listí, šmouhy větru, prach), omráčení
  dopad (prach, kamínky; letec peří) s body, strom padající listí. Bez události z pohledu: přistání na plošině prach
  podle rychlosti klesání (usednutí při vznášení ne), dopad bonusu trocha prachu. `UghEvents::Play` dá každou událost
  v jedné smyčce zvukům i efektům (zvuk i efekt ze stejné události). Niagara ne (binární assety): jako déšť kroku 19
  sítě čtverečků, které hýbe a kreslí GPU - `M_UghBurst` (průsvitný osvětlený), `M_UghBits` (vyříznutý, převrací se
  i se světlem), `M_UghGlint` (světlo), `UghBurst.hlsl`, `UghBurstLook.hlsl`, `UghMakeEffects.cpp`; recepty
  `UghBursts.cpp`, zobrazení `AUghEffects` (od efektu nejvýš 3 naráz, za snímek jen čas, 2 světla záblesků bez
  stínů). Efekty jen dekorace, logika beze změny. Testy `Ugh.Effects.Cues` (každá událost má efekt, smyčka zvuku je
  smyčka efektu, efekty krátké, prach a kouř kryjí nejvýš 0,7), `Ugh.Effects.Events` (událost přes `UghEvents`
  zahraje zvuk i efekt na správném místě, zmizelá entita tam, kde byla naposledy, havárie do vody), `Ugh.Effects.Landing`.
  Snímky `shot.ps1 -Effect all` (`-UghShotEffect`, `-EffectAge`: efekt zastavený u prvního vrtulníku, detail):
  `1p-01-<efekt>.png` pro všech 12 efektů. CTest logiky, `6_verification` (163) a 186 testů v UE zelené.
  `levels.ps1 -Quick` a arch bez vad; fps ve stejném sezení před krokem medián 24 (nejpomalejší 14), po něm 23 (17),
  tedy v šumu. Čeká na Jana: posoudit efekty ve hře (`play.ps1`) - hlavně výbuch, sílu prachu a poryvu foukače
  (`UghBursts.cpp`), zda chce stoupající body (dočasně text v HUD, krok 21 ho předělá). Další: **krok 21**.
- 2026-10-06: krok 21 hotový - menu a HUD (`docs/visual-concept.md`, README hry: `UghUi*`, `UghStoneArt`,
  `UghMenuView`). Místo textu `AHUD::DrawText` obrazovka ve Slate stavěná v kódu (žádné binární widget blueprinty,
  `SUghScreen` ve viewportu, škáluje DPI křivka enginu; navrženo pro 1080 řádků, v 720 čitelné). Písma zdarma bez účtu
  (OFL, Google Fonts na pevném commitu): Lilita One (titulky, čísla) a Alegreya Sans (text) jako nový druh assetu
  `font` v `Assets.json` (`downloads`: soubory i s `OFL.txt`, velikost a SHA-256; `fetch-assets.ps1` je uloží do
  `assets/3d/googlefonts`, import je přeskočí, hra čte TTF za běhu, `package.ps1` je přibalí); bez nich Roboto enginu
  a log (ověřeno shotem). Obrázky kreslí kód při startu (`UghStoneArt`, 35 ms): logo „UGH!“ z tlustých kamenných
  písmen (vzdálenostní pole tahů, každé trochu nakloněné), kamenná deska, kostěný vrtulník života, kost ukazatele
  energie - zkosení, zrno, křivé praskliny, mech nahoře, lem, stín. Titulní obrazovka: kamera pomalu krouží daleko nad
  mořem kolem kamene s vytesaným levelem, který by menu spustilo (`UghMenuView`, místo ztlumeného levelu), vlevo
  ztmavení, logo, panel s řádky Players / Difficulty / Password (pole s kurzorem a levelem nebo „unknown“) / PLAY /
  Quit (vybraný jantarově, šipky ke změně), poslední hra, klávesy v jednom řádku. `FUghMenu` má řádky Play a Quit
  (Enter na Quit končí) a konec hry (`FUghGameEnd`): po hře karta „GAME OVER“ / „ALL LEVELS DONE!“ na kamenné desce
  s levelem, skóre a režimem, dokud se nestiskne klávesa (ta nic jiného nedělá). Hra: dva skleněné panely nahoře
  (level; životy jako kostěné vrtulníky, nad 5 „+N“; energie jako kost se žlábkem zelená / jantarová / červená
  pulzující; skóre s násobitelem), mizí s prolínáním hry; dlouhý řádek kláves pryč - pomoc F1 (sama v levelu 1 přes
  popisek a 6 s hry, F1 ji schová); popisek levelu jako kamenná deska s vytesaným číslem a heslem levelu nad letem
  ke kameni, „Press any key“ pulzuje; stoupající body jantarové s obrysem, vyskočí a stoupají; hlasitost a upscaler
  jako krátké oznámení. Hlášky enginu na obrazovce vypnuté (log je má). Snímky teď obrazovku obsahují
  (`RequestScreenshot` s UI); `shot.ps1 -Menu` (i `menu.png`), `-End` (level vzdán, snímek karty konce hry,
  `-UghShotEnd`), efekt s body (`-Effect shells`) ukáže stoupající skóre. Autopilot jede menu klávesami jako dřív.
  Nové testy `Ugh.Ui.Pictures` (obrázky pokryté, průhledné kolem, stínované; pohled do `Saved\Ui`) a
  `Ugh.Ui.MenuView` (kamera daleko, nad mořem, dívá se na vytesané čelo, plynule), `Ugh.Menu` rozšířený (Play, Quit,
  karta konce). Snímky 1920x1080 prohlédnuté: `menu.png`, `1p-01` a `1p-23` (HUD), `1p-01-intro2.5` (popisek nad
  letem), `1p-03-end` (konec hry), `1p-03-shells` (body), `1p-12` v 1280x720. CTest logiky, `6_verification` (163)
  a 188 testů v UE zelené. `levels.ps1 -Quick` a arch bez vad (HUD na všech, pomoc jen v level 1), fps medián 22
  (nejpomalejší 12; v kroku 20 23 / 17, v šumu). Čeká na Jana: posoudit menu a HUD v okně (`play.ps1`), hlavně logo
  (`UghStoneShapes.cpp`), barvy a velikosti (`UghUiStyle.cpp`), let kamery v menu (`UghMenuView.cpp`); gamepad
  v menu přijde s krokem 22. Další: **krok 22**.
- 2026-10-06: krok 22 hotový - nastavení a ovládání (`docs/visual-concept.md`, README hry). Profil
  (`FUghProfile`) je jeden JSON `Saved\UghProfile.json` (i v zabalené hře; `-UghProfile=<soubor>`; chybějící nebo
  rozbitý soubor = výchozí, zaloguje se): nastavení (`FUghSettings`), klávesy pilotů (`FUghKeyBindings`: ke každé
  klávese logiky dvě, výchozí jako dřív, klávesa jinde se prohodí, vlastní klávesy hry Esc P F1 U G PgUp PgDn, myš
  a gamepad se přiřadit nedají), nejlepší skóre (`FUghHighScores`: 10 nejlepších každého režimu, stejné skóre později
  pod dřívějším, level, obtížnost, den) a level, kam došla poslední hra režimu (jen dohraný, ne startovní z hesla).
  Ukládá se při každé změně, autopilot (`-UghShot`) ho nikdy neukládá a bez `-UghProfile` hraje s výchozím. Menu má
  řádky Settings a High scores; na řádku hesla → doplní heslo posledního dosaženého levelu (pole ukazuje „last: level
  N“), ← smaže. Obrazovka Settings (`FUghSettingsMenu`, `SUghSettingsScreen` ve sloupci titulní obrazovky): kvalita
  Low/Medium/High/Epic (`UghGraphics`: skupiny škálovatelnosti, nízká se stíny a GI Lumenu střední, a těžké věci
  diorámatu jako herní nastavení nad škálovatelností a pod konzolí - objemová mlha, odrazy a lom moře, odrazy Lumenu,
  stínové mapy slunce, objem průsvitných, LOD groomů karty/helmy; Epic = dosavadní vyladění, řádky přesunuté
  z `DefaultEngine.ini` do kódu, výchozí, shoty beze změny), upscaler DLSS/FSR/TSR (bez DLSS jen FSR a TSR), frame
  generation (jen podporované), rozlišení (podporovaná, okno teď), okno (fullscreen / borderless / window; použije se
  až nastavené, ve shotu nikdy), hlasitost celková, hudba, efekty (mixer má hlasitost hudby a efektů zvlášť), let ke
  kameni zap/vyp, Controls; co řádek dělá v panelu pod ním. Obrazovka Controls (`FUghControlsMenu`): tabulka 5 kláves
  × 2 piloti × 2 klávesy, Enter čeká na klávesu (prohození se oznámí, vlastní klávesa hry odmítnuta, Esc nechá),
  Backspace smaže, Defaults; pomoc F1 ve hře ukazuje klávesy, jak jsou přiřazené. Gamepad (`FUghControls` místo
  `FUghKeyboard`; `Config/Windows/WindowsInput.ini` `input.DeviceMappingPolicy=3`, aby všechny gamepady patřily
  jednomu hráči): první gamepad pilot 1, druhý pilot 2 (podle id zařízení), levá páčka nebo d-pad letí, A nebo pravý
  trigger střílí, Start pauza, Back vzdá, Y pomoc; stejné vstupy logiky jako klávesnice (`ugh_logic_key` /
  `ugh_logic_menu_key`, logika beze změny), klávesa logiky držená dvěma klávesami (páčka a d-pad, Space a Right Ctrl)
  se pustí až s poslední; v menu d-pad vybírá a mění, A potvrdí, B zpět (na titulu nic), X maže. Po hře se skóre
  mezi 10 nejlepšími zeptá na jméno (`FUghNameEntry`, řádka kamenných políček na kartě konce hry: psát klávesami, nebo
  jako na automatu ↑↓ písmeno a ←→ posun), pak ukáže High scores (`SUghScoresScreen`: oba režimy vedle sebe, nový
  zápis jantarově, pod každou tabulkou poslední dosažený level s heslem). `shot.ps1 -Screens settings,controls,scores`
  (snímky obrazovek menu klávesami), `-Score <body>` (s `-End`: karta konce se jménem), `-Profile <soubor>` (bez něj
  s `scores`/`-Score` vzorový profil); 1080 řádků přes `-Commands "r.SetRes 1920x1080w"`. Nové testy
  `Ugh.Settings.SaveLoad` (profil uložen a načten stejný, chybějící/rozbitý/divný soubor), `Ugh.Settings.Menu`,
  `Ugh.Settings.Keys` (přiřazení v obrazovce Controls letí v logice, prohození, odmítnutí, výchozí),
  `Ugh.Settings.HighScores` (řazení, 10, rovnost, uložení), `Ugh.Gamepad` (tlačítka 1. a 2. gamepadu dají logice
  cestou hráčova ovladače totéž co klávesy pilota 1 a 2, menu, vrtulník páčkou vyletí stejně vysoko jako šipkou),
  `Ugh.Menu` rozšířený (nové řádky, obrazovky, jméno klávesami i gamepadem, heslo posledního levelu), `Ugh.Sounds`
  (hlasitosti hudby a efektů). Snímky 1920x1080 prohlédnuté: `settings.png`, `controls.png`, `scores.png`,
  `menu.png`, `1p-03-end` (jméno nejlepšího skóre), pomoc F1 v team-01. CTest logiky, `6_verification` (163)
  a 191 testů v UE zelené. `levels.ps1 -Quick` a arch bez vad, fps medián 21 (nejpomalejší 11; v kroku 21 22 / 12).
  Čeká na Jana: vyzkoušet skutečný gamepad (Xbox; žádný tu není - ověřeno jen testem cestou ovladače) a dva gamepady
  v týmu, přepnutí rozlišení a okna v okně (`play.ps1`), jak vypadají předvolby Low/Medium (helmy místo vlasů,
  bez objemové mlhy) a zda Low stačí na 60 fps (krok 23). Další: **krok 23**.
- 2026-10-06: krok 23 hotový - vydání (README hry: Performance, návod `docs/doma.md`). Balení (`package.ps1`) teď
  vaří vše, co hra používá: `Content/Generated`, `Content/Imported`, kopii Electric Dreams a čtyři MetaHumany
  (`DirectoriesToAlwaysCook` po složkách postav, ne postavy Creatoru), pluginy MetaHuman a vlasů se přibalí samy;
  jen DirectX 12 / SM6 (`-D3D11TargetedShaderFormats`: poloviční cook shaderů); vedle data `assets/`, zvuky, písma;
  zip přes .NET (Zip64 - `Compress-Archive` neumí soubory nad 2 GB), velikosti na konci. Balíček 4,16 GB, zip bez
  `.pdb` 3,34 GB (jen pro vlastní použití, nikam se nenahrává). Cook padal na assertu shader mapy (`FAnisotropyPS`):
  master materiály vzorku převádějí atributy do Substrate uzlem legacy conversion s napojenou anizotropií, kterou
  nenastavují - uložená data říkají „bez anizotropie“, překlad „s ní“; `UghElectricDreamsRepair` (v
  `UghCopyElectricDreams` po kopii) vstup Anisotropy odpojí a materiál uloží (4 materiály, vzhled beze změny).
  V zabalené hře opraveny ensure: sítě stavěné za běhu nemají UV hustotu (`UghMeshes::FromDescription` pro skálu,
  listy i ostatní), groomy MetaHumanů bez PSO precache (`PrecachePSOs`). PSO cache (`pso.ps1`, 6 běhů: menu
  a obrazovky, rychlá sada, let ke kameni v každé náladě, efekty ve dne i v noci, karta konce hry, Low a Medium):
  643 stabilních PSO, při startu 172 předkompilováno, 0 bez shaderů. Výkon: hlavní pevná cena snímku na Radeonu 890M
  byly stíny ohňů a loučí (bodová světla = krychle 6 stínových map, kmitající světlo je kreslí každý snímek znovu) -
  zjištěno A/B cvarů ze zabalené hry (CSV profil: render thread čeká na GPU; 25 % rozlišení jen +20 %, bez stínů
  2×). Předvolby (`UghGraphics::Variables`): Low 50 % rozlišení, bez GI Lumenu (sky light), bez stínů ohňů; Medium
  58 %, světla ohňů stojí (`ugh.FireLights.Lick` 0, VSM je cachuje); High jako Epic, ale světla stojí; Epic beze
  změny (67 %, stíny tančí); `r.ScreenPercentage` z upscaleru do předvolby. Hra bez profilu vybere předvolbu podle
  GPU (`RecommendedQuality`: RTX/DLSS Epic, integrovaná Low, jinak High; test v `Ugh.Settings.SaveLoad`). Nápovědy
  kvality v Settings upravené. `perf.ps1` (zabalená hra, rychlá sada, 1920x1080, FSR, mediány fps; před krokem):
  Low 58 (28), Medium 39 (17), High 30 (16), Epic 18 (11); Low nejpomalejší 42, nejrychlejší 62 - cíl 60 jen na
  studeném GPU, po pár minutách plné zátěže notebook zpomalí asi o třetinu (stejná Low pak medián 37). Nové
  `levels.ps1 -Package -Profile -Commands -Tag` (vypíše i chyby z logu), `perf.ps1`, rychlá sada a profil předvolby
  v `ue.ps1`. Celý `levels.ps1 -Package` (150 levelů ze zabaleného, 31 min): všechny snímky, 0 chyb v logu, medián
  25 fps (Epic, 1280x720, zahřátý), oba archy bez vad; rozbalený zip prošel autopilotem bez chyb a ensure. CTest
  logiky, `6_verification` (163) a 193 testů v UE zelené. Čeká na Jana: spustit zip doma na RTX 5060 Ti (automatická
  volba Epic + DLSS, frame generation 2x-4x, plynulost prvního letu), posoudit vzhled Low a Medium, a seznam z kroků
  13-22 v `docs/doma.md`. Další: hotovo (plán 23 kroků vyčerpán).
- 2026-10-06: krok 24a hotový - kostičkovaná zadní stěna vrtulníku. Příčina: proutí (`Blender/copter_materials.py`)
  byl jeden oboustranný čtverec s maskovaným materiálem (díry vyříznuté alfou, glTF MASK). Díry jsou jemnější než
  texel virtuální stínové mapy slunce, hloubka stínové mapy tak skáče mezi stěnou a skálou za ní a paprsky SMRT berou
  ty skoky za stínící hranu - stěna stínila sama sebe v blocích se schody (zblízka i z herní kamery velké tmavé
  skvrny). Ověřeno A/B cvarů na snímku zblízka: bez VSM (`r.Shadow.Virtual.Enable 0`) stín hladký, bez GI Lumenu
  kostičky dál, jemnější VSM jen menší kostičky. Oprava u zdroje: proutí neprůhledné, mezery mezi pásky tmavé
  v barvě (normálová mapa beze změny), materiál bez alfy - stínová mapa vidí souvislou plochu, odpadá i maskovaný
  Nanite a blikání děr pod TSR/FSR. Snímky `1p-01` a `-CloseUp` před a po: stíny na proutí měkké jako s klasickými
  stíny, bez bloků (vrtulník při snímku stoupá a klesá). CTest logiky, `6_verification` (163) a 193 testů v UE zelené.
  `levels.ps1 -Quick` a arch bez vad, 0 chyb v logu; fps medián 12 (nejpomalejší 8; krok 22: 21 / 11) - nejspíš
  zahřátý notebook po sérii běhů (neprůhledný materiál je levnější než maskovaný), A/B neměřeno. Další: **krok 24b**.
- 2026-10-06: krok 24b hotový - vrtulník: šlapání a sezení (`docs/visual-concept.md`). Pilot sedí z profilu (natočený
  o 70°, dřív 50°) na koženém sedátku s opěradlem na bambusových nohách (místo pařezu), klika s poloměrem 12 cm (dřív
  8) a na straně ke kameře velké kolo s kostěnými paprsky a 18 zuby; řetěz z kostěných článků vede nahoru na pastorek
  se 6 zuby na předlohové hřídeli pod střechou, její lucernové kolo točí korunové kolo na hřídeli rotoru (převod 3:1 =
  `RotorTurnsPerPedal`, takže rotor, pastorek, řetěz, kolo, pedály a nohy jdou spolu). Cestující má vlastní židli
  (sedák ze štípaných kmenů s koženým polštářem, opěradlo z kostěných žeber, područky s kly). Čísla v
  `copter_layout.py` / `UghCopterModel.h` (kolo a pastorek odvozené: řetěz kolmo na osu kliky, hřídel míří na osu
  rotoru); nové sítě `Shaft`, `Drive`, `ChainLink`, články jako instance posouvané každý snímek po dráze řetězu
  (`FUghCopterChain`). Proutí dál neprůhledné (24a), tělo v ±45. Akce MetaHumanů a jeskynního muže znovu (větší
  klika). Nový test `Ugh.Copter.Chain`, `Ugh.Copter.Model` kontroluje točící se díly po vrcholech v každém úhlu
  a rozměry kol podle čísel řetězu. `shot.ps1 -Land` (autopilot nechá vrtulníky pomalu přistát; s `-At 8`). Snímky
  před a po v `Saved\Shots\24b-before` a `24b-after` (let, přistání, zblízka, tým, kámen na židli). CTest logiky,
  `6_verification` (163) a 192 testů v UE zelené. `levels.ps1 -Quick` a arch bez vad, 0 chyb v logu, fps medián 27
  (nejpomalejší 18; po 24a 12 / 8 byl zahřátý notebook, A/B netřeba). Čeká na Jana: posoudit nový vrtulník v okně
  (`play.ps1`). Další: **krok 24c**.
- 2026-10-06: krok 24c hotový - čitelnost: postavy vyniknou nad skálou (`docs/visual-concept.md`, Čitelnost). Skála
  tmavší (vápenec `UghCliff.hlsl` 0,16 místo 0,22), expozice světlejších nálad níž (EV100 den 2,1, večer 1,6, soumrak
  1,4, bouřka 1,15; noc beze změny): střední jas snímků o 13-22 % níž. Postavy (lidé, piloti, cestující, nepřátelé,
  bonusy, kámen s očima; `UghFigureLook::Mark`) ve světelném kanálu 1, v něm jen dvě směrová světla `AUghStage` -
  výplň zepředu zprava a kontra zezadu (bez stínů, bez GI Lumenu, jas `FUghMood::FigureFill` x 2^EV, takže stejně
  v každé náladě i na Low). Jemná tmavá svatozář kolem postav: post-process `M_UghFigureHalo` (`UghFigureHalo.hlsl`)
  z custom depth, tři prstence vzorků, ne na tmavém pozadí, ne u lidí ve vrtulníku (`FUghMood::Halo`). Nejdřív byla
  před upscalerem (`AfterDOF`) a skoro nebyla vidět: TSR/FSR drží historii nehybného pozadí a svatozář pohybující se
  postavy rozmaže - teď `BeforeBloom` (za upscalerem). Kužel světla nad čekajícími netřeba: s výplní a svatozáří je
  čekající vidět i v bouřce a v noci (v noci dřív pilot ani nebyl vidět). Nový test `Ugh.Figures.Look` (osoba ve
  světle i custom depth, ve vrtulníku bez svatozáře; světla scény kromě slunce jen na postavách), `Ugh.Mood` hlídá
  čísla postav. Archy před a po: `Saved\Shots\Levels-24c-before`, `Levels-24c-after`, Low `Levels-24c-low`, později
  v levelu (víc cestujících) `Saved\Shots\24c-after`. Low má bílé moře (odrazy jen obloha) už od kroku 23 - beze
  změny. Replaye: UE pouští 161 replayů = 161 souborů `.ugr` na disku; 163 v `6_verification` je 161 replayů,
  `keyboard` a `unit` logiky, žádná mezera. CTest logiky, `6_verification` (163) a 195 testů v UE zelené (161
  replayů a 34 dalších). `levels.ps1 -Quick` 0 chyb v logu, fps medián 27 (nejpomalejší 16; před 26 / 15), Low 67 / 38.
  Čeká na Jana: posoudit v okně (`play.ps1`), zda je svatozář dost jemná a den ne moc tmavý. Další: **krok 24b2**
  (pak 24d).
