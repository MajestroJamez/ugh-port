# Fáze 3 – zvuk

Stav: logika zvuku převedená a ověřená (`SoundTimer.kt`, `SoundDriver.kt`, `SoundLibrary.kt` v `:core`);
syntezátor OPL2 (`core/audio`), výstup přes Java Sound a okno na portu (`desktop`). Emulátor originálu umí AdLib (`3_kotlin_port/oracle/AdLib.kt`: registry, časovače
kvůli detekci, záznam zápisů), zapíná se `OriginalUgh(exe, adlib = true)`; výchozí je stále bez karty, takže
všechny dosavadní testy běží beze změny. `SoundExploreTest` (`.\gradlew.bat :verify:test --tests
ugh.verify.SoundExploreTest -Pugh.explore`) zapíše vykonané adresy zvukového kódu a zápisy do AdLibu do
`4_test_data/verify/build/verify-out/`. Výpis úseku listingu: `node 2_reverse_engineering/tools/asm-range.js <seg> <od> <do>`.

Pozor na listing: `MOV AX,DS` apod. je ve skutečnosti `MOV DS,AX` (Ghidra obrací přesuny do segmentových
registrů). Nové vstupní body (DisassembleAt) vytvořily v 1664 mnoho falešných hlaviček `FUN_`.

## Co hra volá

Jen 6 funkcí knihovny 1878: `0003` init(příznaky, hra dává 1 = AdLib), `008a` konec, `0f88` hudba
(off, seg, délka stmívání, hlasitost), `1059` řízení (0x11 = ztlumit hudbu za 0x11 tiků po 10 Hz a čekat),
`1768` stop kanálu, `1852` efekt (off, seg, hlasitost – v portu zatím pojmenovaná „priority“, příznaky:
0 = smyčka, jinak počet opakování) → vrací číslo kanálu 8–11. Kód pro Sound Blaster (1664:1b4d–2143, 1878:
00f5, 0176, 06e4, 096b, 09ca) a 1878:0f2d, 11ad, 1943, 1a50, 1a8e se s AdLibem nevykoná.

Registry: s kartou knihovna vrací se změněnými BX/CX/DX/ES. `SoundRegistersTest` pouští originál dvakrát se
stejnými vstupy a v druhém běhu je po každém návratu z knihovny přepisuje náhodně – hra se nerozejde (12 000
snímků), zbytek míst volání pokrývá lockstep celého programu s kartou.

## Plánovač časovače 1a32 (DGROUP)

16 slotů (index SI = 0..0x1e po 2): perioda `7a4e/7a6e` (32 bit, tiky PIT), odpočet `7a8e/7aae`, příznak
`7ace`, callback far `7b2e + 2*SI`; handle→slot `7aee`, slot→handle `7b0e`; `7a4c` poslední slot;
`7b6e/70` uplynulo od minulého tiku; `7b76/78` nejkratší perioda = dělič PIT; `7b7a` naprogramovaný dělič.
- `0008` instalace (CS:[6]), slot 0 = BIOS INT 8 přes `0099`, perioda 0x10000. `00a0` odinstalace.
- `01aa` přidání (AX:DX callback, BX:CX perioda) → handle; `021b` odebrání; `02c1` nová perioda.
- ISR `00d2`: odečte uplynulý čas, splatné sloty volá od nejvyššího, EOI, přeprogramuje PIT.
- **Chyba originálu:** `021b` při odebrání jiného než posledního slotu kopíruje callback s nevynásobeným
  indexem a přepíše `7aee[0]`. Hra odebírá jen poslední slot (stmívání), ale port ji zachová.
- Sloty ve hře: 0 BIOS, `1664:1939` 0x1843 (192 Hz; hudba mění periodu = tempo), `1664:12e5` 0x1d1eb (10 Hz,
  stmívání, jen během něj).

## Driver 1664 (data v segmentu 1664)

- `112b` nuluje 000c–0493. `113f` kalibrace rychlosti CPU přes dvě IRQ 0 → `1127`/`1129` (smyčky na tik),
  `11b5` zpoždění; v portu neměřitelné → v lockstepu synchronizovat 1127–112a z originálu.
- `11f2` zápis OPL (BH reg, BL hodnota), `1a37` ticho, `1a89` detekce a init AdLibu (+ instalace plánovače
  a slotu 1939), `1a65` konec.
- Hlasy 0–8: tabulka `0278 + v*0x14` (+0 nástroj 12 B, +0xc hlasitost, +0xe úroveň, +0x10 vlastník,
  +0x12 frekvence A0/B0), stínová `032c + v*0x14` pro hlasy, které si vzaly efekty (hudba zapisuje tam).
  Operátory `10f5`: 0 1 2 8 9 a 10 11 12.
- `1211` hlasitost (škáluje TL nosiče, při aditivní syntéze i modulátoru), `1377` nástroj, `14b1` key off,
  `1503` frekvence, `1571` úroveň.
- Kanál n (8–11 efekty, 12 hudba) = struktura v `n*0x2a`: +0x18 opakování, +0x1a/1c začátek, +0x24/26
  pozice, +0x30 hlasitost, +0x32/34 čas další události, +0x36 počet hlasů, +0x38 první hlas, +0x3a tempo
  (0x100), +0x3c příznaky (1 hraje, 2 smyčka, 4 načteno). Globální: `0236` busy, `023a/c` čítač tiků,
  `0246–024c` stmívání, `0256` hudba aktivní, `0258` stmívání běží, `025a` hlasitost hudby, `026a`
  zařízení, `026c` hlasy rezervované efekty, `0274` aktuální kanál, `000c` tempo hudby, `10ef` struktura.
- Tik `1939`: čítač++, při busy konec; hudba (jedna skupina událostí na tik, interpret `15d3`, tabulka
  CS:174b), efekty jen když `026c` ≠ 0 (interpret `176b`, tabulka CS:1919, časy škálované tempem).
- Bajtkód (horní nibble): 0 dlouhá pauza, 1 frekvence, 2 key off, 3 úroveň, 4 nástroj (11 B), 5 nic,
  6 tempo, 7 meta (0x77 konec stopy / opakování), 8–f pauza 0–127.
- **Chyby originálu:** `0cfc` při nedostatku hlasů vrátí -1 a nechá `0236` = 1 (zvuk pak stojí);
  `0691` bere neúspěch `0aa9` (-1) jako úspěch; efekt 4x povolí hlas 9 a zapíše vlastníka do stínové
  tabulky; efekt 6x dělí bez kontroly přetečení.

## Port a ověření

- Port pracuje nad stejnou pamětí (segment 1664, 2a37, proměnné plánovače v DGROUP) a porovnává se celá;
  výstup jde přes `Host`: `adlib(reg, value)`, `adlibPresent()`, `timerDivisor()`, `timerWait()`.
  Hostitel volá `timerInterrupt()` v rytmu PIT; obsluha jde podle vektoru INT 8 (BIOS = nic).
- Neporovnává se jen 1664:1127–112a (rychlost CPU naměřená driverem, ovlivňuje jen zpožďovací smyčky).
- Lockstep: originál i port mají AdLib, porovnávají se i zápisy do OPL po snímcích. IRQ 0, které originál
  obslouží na hranici snímku (nebo při doručení klávesy), spustí stejný počet `timerInterrupt()` v portu.
  Čekání knihovny na konec ztlumení (1878:10f3) dostane v originálu jedno IRQ 0 za průchod, port ve stejné
  smyčce volá `waitTimer()`; porovnává se počet tiků. Dlouhý tik (mnoho zápisů se zpožďovacími smyčkami)
  může pustit další IRQ 0 vnořeně – závisí to na rychlosti CPU, port odehraje stejný počet tiků za sebou.
- `SoundLibraryTest`: náhodné posloupnosti volání knihovny (efekty všech 10 bloků s různou hlasitostí a
  opakováním, stop libovolného kanálu, hudba, ztlumení) a tiků, krok po kroku v originálu (far call / INT 8)
  i v portu: 6 000 kroků, 123 247 zápisů do OPL, vše shodné. Pokrývá krádež kanálu, stínové hlasy, přesun
  hlasů po konci efektu, opakování stop.
- Lockstep s kartou: celý program 13 814 snímků, konec hry 41 542, hraní 20 000 + 12 000, game over – 0 neshod.
- Nepokryté větve jsou s daty této hry nedosažitelné (aditivní nástroje, meta události mimo 0x77, hudba bez
  opakování, tempo efektu před první hudbou, odebrání jiného než posledního slotu časovače, streamování).

## Syntezátor a výstup

- `core/audio/Opl2.kt`: vlastní OPL2 (49 716 Hz) podle popsaného chování čipu – tabulky logsin/exp počítané
  ze vzorců, obálka 9 bitů po 0,1875 dB s rychlostmi 0–63 (KSR), KSL, tremolo 3,7 Hz, vibrato, 4 tvary vln,
  zpětná vazba, FM i aditivní spojení. Bicí režim, CSM a časovače jako zvuk chybí (hra je nepoužívá: BD ani
  08 nikdy nezapisuje). `Opl2Test`: frekvence z F-number/bloku/násobiče, útlum TL, náběh a uvolnění, FM.
- `core/audio/SoundTimeline.kt`: čas PIT řídí tiky časovače i vzorky; hostitel volá `frame()` jednou za
  snímek (17 024,6 tiků PIT) a `untilNextTick()` z `timerWait()`.
- Hra budí OPL potichu (útlum 18–33 dB, skutečný AdLib to dohnal zesilovačem): výstup se zesiluje 8×, špičky
  v hudbě i efektech pak zůstanou pod plným rozsahem.
- `SoundRenderTest` vykreslí hudbu menu, levelu a rekordů, všechny efekty a mix do
  `4_test_data/verify/build/verify-out/sound/*.wav` (k poslechu; obsahuje hudbu hry, necommitovat).

## Okno

- `3_kotlin_port/desktop/AudioOut.kt`: převzorkování 49 716 → 48 000 Hz (lineárně), linka s bufferem 200 ms, cíl 60 ms ve
  frontě; rozdíl hodin snímků a zvukové karty vyrovná zahozením nebo zopakováním několika vzorků. Busy wait
  hry (ztlumení hudby) zapisuje blokujícím zápisem, takže trvá skutečný čas.
- Okno spouští port z čistého obrazu EXE: C runtime před `main` zapisuje jen prostředí, PSP, své proměnné,
  tabulku souborů a haldu, na kterých hra nezávisí – lockstep celého programu z čistého obrazu (`Lockstep.run(exe)`)
  dává 0 neshod. `--original` dál pouští originál v emulátoru.
