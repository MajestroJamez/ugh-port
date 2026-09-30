# UGH! — fáze 1: mapa UGH.EXE

Stav k 2026-09-30. Adresy jsou v zápisu Ghidry (načítací báze = segment `0x1000`).
Převod na offset v souboru: `file = 0xA00 + (seg - 0x1000) * 16 + off`.

## EXE v kostce

- 16bitový DOS MZ, 411 838 B, **nekomprimovaný**, hlavička 0xA00 B, 525 relokací.
- Runtime **Borland C++ 1991**, ale hra je z velké části **ručně psaný assembler** (TASM, argumenty v registrech,
  NOP výplně po skocích). Dekompilát Ghidry je proto u segmentu 113b jen orientační, závazný je disassembler.
- Kód má jen ~42 kB (225 funkcí), zbytek (~330 kB) jsou data přilinkovaná do EXE. Žádné externí datové soubory
  kromě `UGH!.HI`.

## Segmenty

| Segment | Blok Ghidry | Velikost | Obsah |
|---|---|---|---|
| 1000 | CODE_0 | 5 048 | Borland C startup + RTL |
| 113b | CODE_1 | 21 128 | **hra** (asm): menu, herní smyčka, logika, grafika Mode X, ICE depacker, klávesnice |
| 1664 | CODE_2 | 8 515 | zvukový driver (asm): AdLib OPL2 `0x388`, Sound Blaster DSP + DMA ch1, IRQ |
| 1878 | CODE_3 | 7 069 | zvuková knihovna, C API nad 1664 (init podle příznaků, přehrávání ADLX) |
| 1a32 | CODE_4 | 848 | multi-rate plánovač nad INT 8 / PIT |
| 1a67 | CODE_5 | 64 768 | BSS (nuly): buffer pro rozbalení ICE obrázku (320×200 + paleta) |
| 2a37 | CODE_6 | 16 | data modulu 1878 (příznaky init) |
| 2a38 | CODE_7 | 15 984 | **ICE: mapy všech levelů** (32 000 B = 100 × 320 B) |
| 2e1f | CODE_8 | 55 920 | pixely spritů 0x000–0x0D8 (dlaždice 16×12 aj.) |
| 3bc6 | CODE_9 | 768 | herní paleta (použito 128 barev, 6bit VGA RGB) |
| 3bf6 | CODE_10 | 6 720 | ICE obrázek 320×200 + paleta (intro) |
| 3d9a | CODE_11 | 8 256 | ICE obrázek: logo **Play Byte** |
| 3f9e | CODE_12 | 400 | ICE obrázek (téměř prázdný, intro) |
| 3fb7 | CODE_13 | 10 816 | ICE obrázek: titulní **UGH!** |
| 425b–4637 | CODE_14–23 | 80–7 760 | 10× **ADLX/AHDR** AdLib hudba a zvukové efekty |
| 47a7 | CODE_24 | 48 688 | pixely spritů 0x0D9–0x129 |
| 538a | CODE_25 | 44 944 | pixely spritů 0x12A–0x222 |
| 5e83 | CODE_26 | 52 736 | pixely spritů 0x223+ |
| 6b63 | CODE_27 | 2 656 | **tabulka spritů**: 664 × {u16 offset, u8 šířka, u8 výška} |
| 6c09 | CODE_28 | 32 302 | DGROUP: stav hry, tabulky levelů, hesla, texty, hi-score default |

## Formáty dat

**ICE!** (Pack-Ice 2.x z Atari ST/Amigy). Hlavička `"ICE!"`, packed len (BE32, včetně hlavičky), unpacked len (BE32).
Rozbaluje se odzadu. Depacker je na `113b:4cad–4ddc`, tabulky na `113b:4ddd–4e18`.
Věrný port: `re/tools/unice.js`.

**Obrázek** (ICE, 64 768 B) = 64 000 B chunky pixelů 320×200 (4 po sobě jdoucí pixely = 1 adresa Mode X přes 4 roviny,
`113b:4634`) + 768 B paleta (`113b:4c81`).

**Mapa levelu** = 320 B = 20 × 16 dlaždic po řádcích, hodnota = číslo spritu. Dlaždice 16×12 px, hrací plocha 320×192.

**Sprite**: záznam v CODE_27 `{offset, w, h}`, data v bance podle čísla spritu (hranice 0xD9 / 0x12A / 0x223),
ukládání **po sloupcích**, barva 0 = průhledná. Kreslí `113b:41fd` (AX = sprite, BX = x, CX = y).
Pod hladinou vody (`[2903]`) kreslí `color | 0x40`, protože paleta 64–127 obsahuje „podvodní“ odstíny.
Paleta 0–127 se při startu zkopíruje i do 128–255 (`113b:466a`).

**Tabulky levelů** (DGROUP): `[2909]` → 1 hráč (`0x3349`, **69 levelů**), `[290b]` → team mode (`0x33d5`,
**81 levelů**), ukončeno `0xFFFF`. Záznamy jsou sdílené, dohromady 81 unikátních po **26 B**:

```
+00 u16  offset mapy v rozbaleném CODE_7
+02 u16  ukazatel (DGROUP) - seznam objektů A
+04 u16  ukazatel - seznam B
+06 u16  ukazatel - seznam C
+08 u16  počet (1/2) - zatím neznámé
+0a u16  ukazatel - seznam D
+0c u8   ?
+0d u16  start X hráče 1 (fixed point, 1/32 px)
+0f u16  start Y hráče 1
+11 u16  start X hráče 2
+13 u16  start Y hráče 2
+15 ..   zbytek zatím neznámý
```
Záznam se kopíruje do `DGROUP:28e9` (`113b:3976`). Souřadnice jsou **fixed-point s 5 bity za desetinnou čárkou**
(`SAR AX,5`), jiné jednotky než pixely musí port zachovat.

**Hesla**: ukazatele `[0xe8 + 2*hráči]`: 1 hráč od `DGROUP:00ee` (69 ks), team od `DGROUP:0492` (81 ks),
řetězce oddělené 0, seznam ukončen `0xFF`.

**UGH!.HI**: 5 záznamů × (14 B jméno + 6 B skóre ASCII + CR), celé XOR 0xFF. Výchozí tabulka je v DGROUP
(kolem `DGROUP:0c85`).

## Hardware

- **Grafika**: `int 10h` AX=0x13, pak Mode X (`113b:503b`): chain-4 off, byte mode, CRTC offset 0x30
  (virtuální šířka **384 px**, 96 B na řádek a rovinu), line compare 0x17F (split screen = stavový řádek),
  page flip přes CRTC 0x0C/0x0D (`113b:4ebb`). Fade palety `113b:4e19/4e28/4e36`.
- **Časování**: herní smyčka čeká na **vertikální zatemnění** (`113b:44c3`, port 0x3DA bit 3) → 1 tik = 1 snímek VGA
  = **70,086 Hz**. Hra je na snímky pevně vázaná.
- **PIT/INT 8** (`1a32`): plánovač callbacků pro zvuk: `1664:12e5` @ 10 Hz, `1664:1939` @ ~192 Hz.
- **Klávesnice**: v menu BIOS `int 16h`, ve hře scancody (`113b:44f1`), pravděpodobně vlastní INT 9 (ověřit).
- **Joystick**: port 0x201 (`113b:50c6`, `113b:51ec`) - v portu vynecháme.
- **Zvuk**: AdLib OPL2 (hudba i efekty ve formátu ADLX), Sound Blaster DSP/DMA v driveru (zda se používá, ověřit).

## Tok programu

```
entry (1000:0000, Borland c0)
 └ main = 113b:0008
    ├ 113b:4f5e  int 10h - uložit režim, nastavit 13h
    ├ 113b:0bb8  intro: 4 ICE obrázky (Play Byte, …, UGH!), ESC = přeskočit, Q = konec
    ├ 113b:503b  Mode X
    ├ 113b:0063  hlavní menu (scancody):
    │    F1 (3B) start hry → 113b:0c07
    │    F2 (3C) heslo → 113b:04ca
    │    F3 (3D) obtížnost EASY/MEDIUM/HARD  ([2638] = 0..2)
    │    F4 (3E) ONE PLAYER / TEAM MODE      ([2634] = 1/2, [2636] = 2*[2634])
    │    F5 (3F) ovládání KEYBOARD/JOYSTICK A/B pro hráče 1 a 2
    │    C  (2E) ?, Q (10) konec, nečinnost 0x834 snímků → demo/credits
    └ 113b:4ee6  návrat do textového režimu, exit

herní smyčka (113b:0c07, jeden průchod = jeden snímek 70 Hz):
    fade krok (4e36) nebo čekání na vsync (44c3)
    3f55  vykreslení + flip
    436d  vstup hráčů (pro každého hráče)
    P (19) pauza, ESC (01) konec levelu (fade-out)
    1095  aktualizace hráčů (vrtulníky) - pro každého hráče
    1486, 2363, 2b7f  aktualizace objektů (nepřátelé, pasažéři, ovoce…)
    3976  načtení levelu (tabulka, mapa, start pozice)
```

Část 113b (`2e03–3961`) tvoří **kompilované sprity**: rozvinuté `MOV [DI+n],reg`, grafika uložená jako kód
(animovaná hladina vody, 3 snímky, `113b:2db9`).

**Oprava (fáze 3):** tvrzení, že je disassembly kompletní, neplatilo. Ghidra se zastavila na nepřímých skocích
a ~10 kB herní logiky zůstalo nedisassemblovaných. Doplněno ve fázi 3, viz [phase3-oracle.md](phase3-oracle.md).

## Nástroje

- Ghidra 12.1.4 v `C:\Apps\ghidra`. **Musí běžet s JDK 23**, protože přibalený Felix 7.0.5 na JDK 25 padá
  („The data file must be inside the data dir“). `java_home.save` v `%APPDATA%\ghidra\ghidra_12.1.4_PUBLIC`
  je přepnutý na `~\.jdks\temurin-23.0.2`.
- `re/tools/run-ghidra-script.ps1 <Script.java>` - pustí post-script nad analyzovaným projektem.
- `re/ghidra-scripts/ExportOverview.java` - bloky, funkce, INT/IN/OUT, dekompilát → `re/out/`.
- `re/ghidra-scripts/ExportListing.java` - plný disassembly + kdo používá jaký blok.
- `re/tools/mzinfo.js`, `blocks.js` - hlavička, relokace, charakteristika bloků.
- `re/tools/unice.js`, `extract-ice.js` - depacker a vybalení obrázků.
- `re/tools/render-level.js` - vykreslení všech levelů (mapa dlaždic) do PNG.

## Otevřené otázky (fáze 2-3)

1. Seznamy objektů v záznamu levelu (A-D): ovoce, obydlí/zastávky, nepřátelé (dinosaurus, pterodaktyl…), voda.
2. Co přesně mění obtížnost `[2638]`.
3. Fyzika vrtulníku (`113b:1095`): gravitace, šlapání, energie, přistání, kolize s dlaždicemi.
4. Generátor náhodných čísel a jeho seed (nutné pro deterministický replay).
5. Mapování ADLX → přehrávání (formát instrumentů a patternů), 10 skladeb/efektů, a zda se používá SB.
6. Demo režim v menu (záznam vstupů? přímo použitelné jako referenční test).
7. Význam klávesy C (0x2E) v menu a obrazovek mezi levely.
