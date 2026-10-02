# Plán: UGH! remake v UE 5.8 - krok za krokem (1 krok = 1 session)

Kontext a výsledky zkušebního průchodu: [phase4-modernization.md](phase4-modernization.md).
Pravidla platná pro všechny kroky:

- Fyzika a logika **přesně jako originál**. Kotlin port (`core/`) je jen reference a generátor testů, nepřepisuje se.
- Testy jsou data: golden replays `UGR 0` (`verify/src/test/kotlin/ugh/verify/replay/`), každá nová
  implementace musí projít všemi replayi pole po poli.
- Herní data se necommitují (jen kód); C++ jádro čte data vytažená extractorem do `assets/`.
- Nic viditelného na notebooku bez souhlasu (okno hry, editor se scénou, Blender); příkazy pro Jana jen PowerShell 5.1.
- Na konci každého kroku: testy zelené, krátký zápis do tohoto souboru (sekce Stav), commit po Janově souhlasu.

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

## Krok 9 - UE projekt v repu, šedé kostky

- `game/` (UE 5.8 C++ projekt), C++ jádro jako UE modul (stejné zdrojáky), pluginy DLSS/FSR jako v UghTrial
  (FSR jen upscaler: `r.FidelityFX.FI.Enabled=0`, `OverrideSwapChainDX12=0`; offscreen oprava FSR).
- Level z mapy dlaždic a kolizní masky jako jednoduché kostky; vrtulník, cestující, nepřátelé jako tvary.
- Pevný tik 70,086 Hz + interpolace pro vykreslení; ovládání klávesnicí (písmena, kvůli české klávesnici).
- Replays jako UE automatické testy (`UnrealEditor-Cmd -nullrhi`).
- Hotovo když: level 1 jde odehrát a replays projdou i uvnitř UE.

## Krok 10 - Vizuální směr „Pravěké dioráma“

- Krátký koncept (paleta, materiály, světlo, kamera), pak první level: útes v řezu generovaný z mapy dlaždic,
  ohniště s Lumen/RT, voda, mlha; herní rovina zůstává přesně podle kolizní masky.
- PSO cache pro balení (bez trhání na startu), skript na balení (`NO_PROXY += ::1`).

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
