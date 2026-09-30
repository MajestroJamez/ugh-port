# UGH! — fáze 3: disassembly, vstupy a emulátor originálu

Stav k 2026-09-30.

## Doplnění disassembly

Herní logika je řízená **tabulkami obslužných rutin** (stavové automaty, kde rutina přepisuje ukazatel na další
stav), takže je flow analýza Ghidry nenašla:

| Tabulka | Kdo ji prochází | Zdroj adres |
|---|---|---|
| `DGROUP:2a0d[i]` | pasažéři, `113b:1486` | `[deskriptor+8]` / `[deskriptor+0x10]`, stavy v tabulkách deskriptorů `DGROUP:772a…` |
| `DGROUP:2cf3[i]` | objekty, `113b:2363` | `2a87`, `2379`, `295b`, `25b1` (loader `3b21`) a stavy v `DGROUP:763a…`, `7676…`, `76c4…` |
| `DGROUP:2d83[i]` | další skupina, `113b:2b7f` | stavy v `DGROUP:78ec…`, `7976…` |
| `CS:2db3` | voda, `113b:2dfe` | `2e03`, `31cd`, `3597` (3 kompilované snímky) |

Postup: `re/tools/find-code-pointers.js` sbírá kandidáty (konstanty zapisované do tabulek a slova v deskriptorech
ukazující do nepokrytého kódu, s kontrolou, že před nimi je `RET`/`JMP`). `re/ghidra-scripts/DisassembleAt.java`
je disassembluje (a umí vyčistit chybně disassemblovaný rozsah). `re/tools/coverage.js` hlásí nepokryté bajty.

Výsledek: segment 113b je pokrytý celý kromě tří potvrzených datových oblastí (`460a–4634` CS proměnné,
`4690–4c81` buffery palety, `4dea–4e19` tabulky ICE). Ve zvukovém driveru 1664 zbývá ~5 kB, doplní se se zvukem.

## Vstupy

- **INT 9** `113b:4567`: uloží syrový scancode do `CS:4509` (menu, pauza, `113b:44f1`), porovná sekvence
  s tabulkou `DGROUP:281c` (6 B: scancode, druhý bajt/0xFF, 0xFF, slot, hodnota, stav) a zapíše
  `DGROUP:278c + slot` = 0xFF (stisk) / 0x00 (puštění). Sloty se sudým bitem 1 patří hráči 1, s lichým hráči 2
  (a platí jen s ovládáním klávesnicí, `DGROUP:263f`).
- **Hráč 1:** šipky (s prefixem E0) + pravý Ctrl / Ins. **Hráč 2:** W / Z / A / S + levý Ctrl / Enter na numerické
  klávesnici (šipky na numerické klávesnici bez E0 také hráč 2).
- Menu čte změnu posledního scancode (`44f1`: `AH` = změna, `AL` = scancode).

## Náhoda a čas

- **RNG** `113b:4f09`: aditivní generátor nad 4 slovy v `CS:4ef7–4efe`, počáteční stav je v EXE a nikde se
  nepřenastavuje. Výsledek = `(stav * AX) >> 16`, tedy číslo 0 až AX−1.
- Herní smyčka čeká na vertikální zatemnění (`44c3` = počkat na konec zatemnění `44d2` a pak na začátek `44c9`).
  **1 herní snímek = 1 volání `44c3`.** Průběh hry závisí jen na vstupech po snímcích.
- Hra vyžaduje **286+** (`113b:4f5e` testuje horní bity FLAGS).

## Emulátor originálu (`:oracle`)

Kotlin modul, který spouští originální `UGH.EXE` deterministicky a bez okna:

- `Cpu`: interpret 80286 v reálném režimu (sada 80186, chování FLAGS jako 286).
- `Vga`: planární paměť Mode X, latche, zápisové režimy 0–3 s rotací, ALU a bit mask, čtecí režimy 0–1, DAC,
  CRTC (start address, offset, line compare → split screen), vykreslení snímku.
- `Machine`: 1 MB RAM, načtení MZ na segment 0x1000 s relokacemi (**stejné adresy jako v Ghidře**), PSP
  a prostředí, PIC, PIT kanál 0, klávesnice (fronta scancode → IRQ1), BIOS (INT 8, 9, 10h, 16h, 1Ah) a DOS
  (INT 21h: vektory, paměť, soubory ve virtuálním disku) v Kotlinu. Zvuková karta chybí, takže hra běží bez zvuku.
- **Čas = počet instrukcí** (20 M/s). VGA 70,086 Hz a zatemnění 2 řádky, PIT podle děliče. Nic nezávisí na
  hodinách hostitele. Čekací smyčky na VGA (`44c9`, `44d5`) se přeskakují, takže 1 herní snímek při čekání stojí
  jen pár instrukcí. Celá hra běží ~30× rychleji než reálný čas.
- `OriginalUgh`: počítá herní snímky, `runGameFrames`, `runUntil(seg, off)`, `tap(scancode)`, `startGame()`
  (menu → F1 → přeskočení animace a popisku → první snímek levelu), přístup k DGROUP a screenshot.

Ověřeno: intro (4 obrázky), menu, úvodní animace, popisek levelu i hra v levelu 1 vypadají správně.
Od zapnutí po první snímek levelu 1 je to 11 728 herních snímků (intro 11 427).

## Přesné pokrytí disassembly

Ghidra na několika místech vynechala i krátké úseky (např. `113b:1a06–1a0c` uprostřed rutiny `19fb`), které
dřívější odhad pokrytí přehlédl. `ExportListing.java` proto u každé instrukce zapisuje délku a bajty
a `re/tools/coverage.js` počítá pokrytí přesně na bajt. Segment 113b je teď pokrytý celý kromě dat
(`2db3` skoková tabulka vody, `4509`, `45fb–4634` CS proměnné, `4681–4c81` palety, `4ddd–4e19` tabulky ICE)
a tří nedosažitelných zarovnávacích `NOP`.

## Port a rozdílové testy

- `:core` obsahuje sdílený adresní prostor (`Memory`, loader MZ) a VGA, takže port pracuje se stejnou pamětí
  jako originál. `Game` = přepsané rutiny.
- `:verify`/`CallDiff`: při každém volání originální rutiny zkopíruje celý stav do portu, spustí port a po
  návratu originálu porovná RAM (kromě zásobníku) a VGA. Přerušení se během kontrolovaného volání odkládají.
  Test s úmyslně pokaženým portem ověřuje, že rozdíly opravdu hlásí.
- `Pilot`: deterministický náhodný pilot. `CheatPilot`: kvůli pokrytí logiky pasažérů zapisuje mezi snímky polohu
  vrtulníku přímo do paměti (přistání na plošině s pasažérem, doručení, přelet přes pasažéra, hladina, shození)
  a mezerníkem pokračuje do dalších levelů.

| Rutina | Port | Ověřeno |
|---|---|---|
| `1095` fyzika vrtulníku, `1457` test kolize | `Game.copterUpdate` | 3 průběhy, 6 000+ volání |
| `1486` pasažéři (36 stavových rutin), `2276` dotyk s vrtulníkem | `Passengers.kt` | levely 1–3, 1 hráč i team, 116 000+ volání |
| `2b7f` bonusy, `2b96` spawn, `2207` sebrání | `Bonuses.kt` | spolu s pasažéry |

### Pasažéři

Stav pasažéra je adresa rutiny v `DGROUP:2a0d[i]`. Rutiny pochází z deskriptoru typu (`29ad[i]`: `0x7720`,
`0x77b4`, `0x7848`, `0x78dc` stojící) a navzájem si předávají řízení `JMP [SI+n]` i s registry (hlavně DI = hráč).
Deskriptor: `+0/+2` posun postavy, `+4/+6` polovina šířky a výšky (dotyk s vrtulníkem), `+8…+2c` stavové rutiny,
`+2e…+38` animační tabulky, `+3e` zpoždění animace, `+40/+42` počáteční a minimální jízdné, `+44` čas plavání,
`+46` typ nákladu, `+48` sada stavů pro vodu. Výplata = jízdné × násobitel `263d` do 32bitového skóre `261e`.
Rychlé doručení (časovač `2c8d`) pustí bonus (deskriptor `7a38`).

### Bonusy

12 slotů (`2d6b…`), padají s gravitací 3, na plošině leží 0x230 snímků. Sebrání: typ 0 = energie `2622`
(max 0x5a3b), typ 1 = životy `263c` (max 99), jinak násobitel `263d` (max podle obtížnosti `2628`).
**Chyba originálu:** když je všech 12 slotů plných, `2b96` vynechá `POP BX` a `RET` skočí na `CS:BX`.
Port v tom případě hlásí výjimku.

## Další kroky

1. Zápis stavu DGROUP po každém snímku a přehrávání vstupů po snímcích (replay).
2. Návrh jádra portu a porovnání s emulátorem: po funkcích (stejný stav → originální rutina vs. port)
   i po snímcích.
3. DOSBox-X jako nezávislá kontrola samotného emulátoru na několika replayích.
