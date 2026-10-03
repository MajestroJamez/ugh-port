# Krok N1: co ze staré paměti logika opravdu potřebuje

Stav k 2026-10-03. Měření na `sim/` (beze změny na `main`; instrumentace je ve větvi `audit/n1`: `sim/src/core/Audit.*`,
`sim/tools/ugh_replay/AuditTable.hpp`, přepínač `ugh_replay --audit`). Všech 161 replayů `UGR 0`, 434 tis. snímků.
Zadání v [rewrite-design.md](rewrite-design.md), kap. 11 N1.

## Metoda

1. **Přirozený běh** (`ugh_replay` v režimu celé hry, počítadla `UGH_AUDIT`): každé sdílené slovo si pamatuje, v jakém
   významu ho kdo naposledy zapsal, každé čtení se porovná; čtení animací mimo seznam; počítadla zvláštností (přetečení
   `Word`, neznaménková porovnání, sonda, déšť, životy, energie ...). Výsledek je 161 × „0 mismatches“ – instrumentace
   chování nemění.
2. **Otrávení** (`ugh_replay --audit`): jádro dostane jen vstupy (tick 0, klávesy, `I` řádky), nic se nepřebírá.
   Po každém snímku se **každé pole, které podle navržené tabulky UGR 1 (kap. 9) v daném stavu není definované**,
   přepíše v obou jádrech dvěma různými hodnotami (`AuditTable.hpp`: tabulka po skupinách, druzích, stavech a fázích).
   Kdyby logika takové pole kdykoli později přečetla, rozejdou se definovaná pole jader nebo nebudou souhlasit
   s replayem. Výsledek ukazuje **postačitelnost** tabulky (chybějící pole); nutnost polí v tabulce je rozebraná
   z kódu (níže).

## 1. Závisí známý stav na zbytcích paměti?

**Ne**, až na tři případy, které jsou skutečný stav hry (ne zbytek), a jeden artefakt testovacího pilota.
Otrávené běhy prošly všech 161 replayů; porušení tabulky (pole, které se rozešlo, protože bylo otrávené):

| Porušení | Výskyty | Příčina | Rozhodnutí |
|---|---|---|---|
| `copter.effort` na začátku `play` | 670 | Při stmívání (fade-in do 3/4) vrtulníky nelétají, ale rotor se točí podle `effort` z **posledního snímku předchozího pokusu** (na začátku hry 0 ve všech 161 replayích). | Pojmenované chování: `effort` je stav vrtulníku po celý pokus i mezi pokusy; nová hra ho nuluje. Pole `copter.N.effort` v `caption`, `setup`, `play`. |
| `copter.impact` na začátku `play` | 670 | Stejné místo, ale `impact` čte jen fyzika ve stejném snímku po `startFrame()` (nuluje ho). | **Není stav** – pomocná hodnota jednoho snímku fyziky. V UGR 1 ani v modelu vrtulníku není. |
| `game.waterRow` v `setup` | 388 | Popisek si řádek vody půjčuje (`0xaf`) a při zatmívání vrací. V logice platí vždy `řádek = hladina >> 5` (`fillTo` i `move`). | **Není stav** – odvozený z `water.level`. Pole `water.row` v UGR 1 nebude, půjčování popiskem se zahazuje. |
| `object.sprite` ve stavech `*Init` (popisek) | 252 | Načtení levelu sprite nepřítele nenastaví – drží zbytek předchozího levelu / úvodního dema; před hrou ho `Playing::enter` skryje. | Nepřítel ve stavu Placed nemá sprite; pole v UGR 1 chybí. |
| `passenger.timer`, `xf` ve `Falling` | 12 | `CheatPilot.injectHit` nastaví stav `Falling` zápisem do paměti, bez rychlosti (zbytek z dema). | Zmizí v N3 (zásah nahradí skutečný pád z vrtulníku). |

Na začátku hry jsou v paměti po úvodním demu i hodnoty, které tabulka nepovažuje za definované (cestující na popisku
prvního levelu: `counter`, `timer`, `vy`, `x` ...; nepřátelé: `anim`, `timer`, `vx` ...). Otrávení prokázalo, že je
logika nikdy nečte – nové jádro je ignoruje. Vstupy nové hry zůstávají jen tři: stav generátoru náhody, řádek
deště (`rainFloor`) a `effort` = 0.

## 2. Čte originál sdílené slovo v jiném významu?

**Ne.** Jediná nesouhlasná čtení (666 + 7) jsou po zásahu `injectHit` (stav `Falling` zapsaný bez
`timer` / `vy`). Ostatní čtení ve stejném významu, v jakém slovo naposledy zapsal stav téže entity:

| Slovo (`sim/`) | Význam | Čtení (stavy) | Počet |
|---|---|---|---|
| `PassengerCounter` | odpočet | Calling, Impatient, SwimCalling, SwimWaving | 127 tis. |
| | místo čekání | Waiting | 9,6 tis. |
| | kdo veze | Boarding, SwimBoarding, Riding, Standing, Hanging | 27,8 tis. |
| `PassengerTimer` | zpoždění příchodu | Arriving | 278 tis. |
| | x rychlost puštěného | Falling (+ strom, který ho odrazí) | 6,4 tis. |
| | čas na hladině | Swimming | 22,6 tis. |
| `Passenger::vy` | pád stojícího | Falling + nepřátelé (`fallingPassengerNear`, odraz) | 9,9 tis. |
| | plavání | Splash, Sinking | 15 tis. |
| `EnemyFacing` | směr walkera | Walking, Watching, Charging, Recovering, Stunned | 39 tis. |
| | cíl flyera | FlyerWait2 (enter Flying), Flying | 138 |
| `EnemyTimer` | odpočet | FlyerWait, FlyerWait2, Watching, Stunned, BlowerWait, TreeWait | 85,7 tis. |
| | rychlost | FlyerFalling (pád), Charging (náběh) | 3,2 tis. |
| `EnemyTable` | animace letu | Flying | 1,9 tis. |
| | další bonus stromu | Tree, TreeWait | 80 |
| `BonusTimer` | x rychlost | bonus Falling | 7,2 tis. |
| | doba ležení | bonus Lying | 10,8 tis. |
| `Passenger::bonusTimer` | čas rychlého doručení | Riding, WalkingAway | – (nikdy nečteno nezapsané) |

Rozhodnutí: rozdělení sdílených slov z kap. 5 platí beze změny; každý význam je vlastní pole s vlastním jménem.

## 3. Čtení animace za koncem seznamu

**Nikdy** (0 čtení s indexem za ukončovací značkou nebo záporným ve všech 161 replayích). Pozice snímku se sice
při přepnutí animace zachovává (Waiting, Recovering, walker vlevo / vpravo), ale vždy na animaci stejné délky nebo
před koncem. Rozhodnutí: UGD 1 žádné „přetečení“ nepotřebuje, `Animation` je jen seznam spritů; `Animator::show()`
dál začíná od nuly na konci seznamu (to je běžné opakování, ne čtení za koncem).

## 4. Definovanost polí po stavech

Otrávení potvrdilo navrženou tabulku kap. 9 s opravami výše; nutnost jednotlivých polí je rozebraná z kódu.
Výsledná pravidla (finální tabulka do kap. 9 v N3):

- **Odvozené pohledy:** `pickupPad`, `targetPad` cestujícího = zastávka `routeStop` jeho trasy (originál je zapíše
  v NextStop a trasu posune v Entering těsně před dalším NextStop) – v UGR 1 jsou, ale v logice nejsou uložené.
- **Poloha „viděná minulý snímek“** (`pixelX/Y` cestujícího, v UGR 1 `seenX`, `seenY`): skutečný stav cestujícího
  s trasou. Obnovuje se na konci snímku, jen když je vidět; skrytý ji drží. Pozor: ve WalkingAway porovná první krok
  pozici dveří s polohou **viděnou při nástupu** (Riding je skrytý), ne s polohou u vrtulníku. Proto `seenX/Y` od
  Appearing až po Entering i ve vodních stavech, včetně Riding; ne v NextStop, Arriving, Gone. Stojící cestující ji
  nepoužívá (pole nemá).
- **`x`, `y` cestujícího:** všechny stavy s polohou (ne NextStop, Arriving, Riding, Hanging, Gone).
- **Animace** (`animFrame`, `animDelay`): stavy, které animují; každý stav cestujícího ji při vstupu restartuje, takže
  ve skrytých stavech není. Flyer ji má i v Hidden a Screeching (Flying pokračuje od restartu ve FlyerWait), strom
  i v Resting (Swaying pokračuje, kde byl).
- **`copter.pixelY`** zůstává stavem: po odhození walkerem je o snímek pozadu a sonda ho čte.
- **`copter.fareMin`** jen s cestujícím s trasou na palubě (`lowerFare`); `copter.fare` vždy (platba, stavový řádek).
- **Konstanty z dat** (`object.pad`, `object.startDelay`, `passenger.startPad`) v UGR 1 nejsou.
- **Bonus `vy`** jen ve Falling (Lying ho nečte – otrávení prošlo).
- Pole hry v `setup` stejná jako v `caption` a `play` (UGR 0 je v `setup` vynechává jen kvůli projekci).

## 5. Zvláštnosti (kap. 6): kde se projeví

| Zvláštnost | Výskyty ve 161 replayích | Rozhodnutí |
|---|---|---|
| 16bitové přetečení (`+`, `−`, `<<`, unární `−`) | **0** | `Int16` / `Fixed` / `Speed` zůstávají (věrnost i mimo replaye, levné), ale přetečení se v replayích nikdy nestane. |
| Neznaménkové porovnání dá jiný výsledek než znaménkové | **0** (16 tis. porovnání) | `unsignedLess` se jménem pravidla zůstává (věrnost); v praxi se hodnoty se znaménkem nepotkají. |
| Sonda doleva / nahoru jen o pixel: tah víc než o pixel | 8,8 tis. | |
| — a plné prohledání by dalo jiný výsledek (vrtulník proletí zdí) | **25× doleva, 242× nahoru** | Pojmenované chování v `CollisionProbe`, má vliv na hru. |
| Bod sondy za okrajem řádku masky (x < 0) | 35,9 tis., **vždy prázdný** | Maska je široká 384 px, dlaždice kreslí jen 0..319, takže sloupce 320..383 jsou ve všech 81 levelech prázdné a „sousední řádek“ nikdy nenarazí. `CollisionMask::solid(x, y)` = mimo 0..319 × 0..191 nic není; UGD 1 nese masku jen 320 px. Shoda ověřená konstrukcí masky (extractor) – zvláštnost zaniká. |
| Bod sondy nad / pod maskou | 277 tis. | Mimo masku nic není pevné (už dnes). |
| Kapka deště přes okraj řádku 384 px (pokračuje na dalším řádku) | 20,7 tis. | Pojmenované chování `Raindrop` (kapka jako x, y se šířkou stránky 384); mění okamžik nového zrodu → náhodu. |
| Kapka mimo obrazovku vpravo (x ≥ 320) | 9,4 tis. | Simuluje se dál (je v kontrolním součtu). |
| Řádek kapky při zrodu > 255 (uložení v bajtu) | **0** | Zahodit: řádek je vždy < hladina < 192. |
| Déšť při načtení levelu s podlahou ≠ hladině levelu | 12 (= 5) | Pojmenované chování „déšť si pamatuje poslední hladinu“ (kap. 6) platí. |
| Životy pod nulou (Esc) | 0 | Pravidlo zůstává (Esc = konec hry po pokusu), replaye ho nepokrývají. |
| Zbývající cestující pod nulou | 0 | Pravidlo zůstává. |
| Energie pod nulou | 0 | Nic se neděje; zůstává `Int16`. |
| Body za doručení > 16 bitů | 0 | 32bitové skóre zůstává. |
| Vrtulník odhozený walkerem (pixelY pozadu) | 29 | Pojmenované chování. |
| 12 bonusů naráz | 0 | `Diagnostics`. |

**Nové zvláštnosti** (doplněné do kap. 6): rotor se při stmívání točí podle `effort` posledního snímku předchozího
pokusu; ve WalkingAway rozhoduje první krok poloha viděná při nástupu; Riding po záchraně z vody uvolní plošinu
vyzvednutí (i když na ní mezitím čeká jiný cestující); `Playing::enter` nechá před hrou proběhnout jeden update
nepřátel a pak cestujících a vše skryje.

## Shrnutí rozhodnutí

| Nález | Rozhodnutí |
|---|---|
| Zbytky paměti (demo, předchozí level) | zahodit – otrávení prokázalo, že je logika nečte |
| Sdílená slova | rozdělit podle kap. 5 – originál je nikdy nečte v jiném významu |
| Animace za koncem | nenastává – UGD 1 bez přetečení |
| `impact`, `water.row`, sprite nepřítele v Placed, konstanty z dat | nejsou stav – mimo model i UGR 1 |
| `effort` mezi pokusy, poloha viděná minulý snímek, `copter.pixelY` | stav – pojmenované chování |
| Přetečení řádku masky | zaniká (sloupce 320..383 prázdné) – maska 320 × 192 |
| Kapky přes okraj, sonda doleva / nahoru, déšť s pamětí hladiny | pojmenované chování (vliv na hru prokázán) |
| Přetečení `Int16`, neznaménková porovnání, životy pod nulou | ponechat jako typ / pravidlo, v replayích se neprojeví |
| `injectHit` | nahradit v N3 |
