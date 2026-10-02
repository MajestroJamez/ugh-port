# UGH! — fáze 2: formát dat a extraktor

Stav k 2026-09-30. Navazuje na [phase1-map.md](phase1-map.md).

## Extraktor

Gradle modul `:extractor` (Kotlin 2.4.20, JDK 25).

```powershell
.\gradlew.bat :extractor:test
.\gradlew.bat :extractor:run
```

`run` čte `OLD/UGH.EXE` a zapisuje do `assets/`, které je v gitignore, protože data jsou odvozená z originálu.
Extraktor odmítne jinou verzi EXE (kontroluje SHA-256 `ef93d2cd…6d7c`), protože všechny offsety platí jen pro ni.

| Výstup | Obsah |
|---|---|
| `palette.vga` | herní paleta, 768 B, 6bit VGA (0-127 + kopie v 128-255) |
| `sprites/NNN.png` + `sprites.json` | 662 spritů jako indexované PNG (původní indexy barev, 0 = průhledná); sloty 662 a 663 jsou prázdné |
| `levels.json` | 81 levelů, pořadí pro 1 hráče (69) a TEAM (81), obě sady hesel |
| `pictures/*.png` | intro: Play Byte, Bones Park, „present“, titulní UGH! (každý s vlastní paletou) |
| `adlx/*.adlx` | 10 surových AdLib bloků (formát zatím nerozluštěn) |
| `dgroup.bin` | inicializovaná část DGROUP (0x7e2e B) pro věrný port tabulek podle původních offsetů |
| `preview/` | náhledy: všechny sprity, všechny levely (jen dlaždice) |
| `manifest.json` | SHA-256 zdroje a počty |
| `sim/ugh-sim.bin` | data herní logiky pro C++ jádro (viz níže) |

Kontroly v extraktoru a testech:
- ICE depacker dává bajtově shodný výstup s nezávislým JS portem (`re/tools/unice.js`), obrázky jsou vizuálně správně.
- Každá ze 4 bank spritů je záznamy tabulky pokryta **přesně do posledního bajtu**, bez mezer.
- U každého z 81 levelů pokrývají bloky (plošiny, trasy, pasažéři, objekty, popisek) data levelu beze zbytku
  a bez překryvů. Pořadí bloků se mezi levely liší.
- Počet hesel = počet levelů v obou režimech.

## Data pro C++ jádro: `assets/sim/ugh-sim.bin` (formát `UGHSIM01`)

Všechno, co potřebuje herní logika, v jednom binárním souboru (little endian, `Sim.kt`):

```
0   char[8]  "UGHSIM01"
8   u32      počet bloků N
12  N x 20 B {char[8] jméno (doplněné NUL), u16 seg, u16 off, u32 pozice dat v souboru, u32 délka}
    data bloků
```

| Blok | seg:off | Obsah |
|---|---|---|
| `DGROUP` | `6c09:0000`, 0x7e2e B | inicializovaná DGROUP: záznamy levelů (26 B, tabulky `3349` 1 hráč / `33d5` team), jejich seznamy A-D (plošiny, pasažéři a trasy, objekty, popisek), deskriptory pasažérů (`7720`, `776a` …, po 0x4a), objektů (`7630`, `766c`, `76a8`, `76e4`) a bonusů (`7a38` …), tabulky animací (sprity, `0d00` …), hesla (`00ee` / `0492`), tabulka kláves (`281c`), limity nárazu (`262e`) a multiplikátoru (`2628`) |
| `MAPS` | `1a67:0000`, 32 000 B | rozbalené mapy levelů (CODE_7): 20 × 16 čísel dlaždic od `záznam+00` |
| `SPRITES` | `6b63:0000`, 664 × 4 B | tabulka spritů `{u16 offset v bance, u8 šířka, u8 výška}` |
| `MASK` (81×) | `6c09:<záznam levelu>`, 9 216 B | kolizní maska levelu, viz níže |

Logika i golden replays odkazují na data **původními offsety v DGROUP** (`passenger.N.kind=0x7720`,
`route=0x…`, `object.N.table=0x…`), proto zůstávají tabulky na svých adresách, ne v novém schématu.

**Kolizní maska:** bit 7 barvy každého pixelu stránky pozadí po nastavení levelu (`113b:3d66`): 384 × 192 px,
řádek po 48 B, pixel x v bitu `7 - (x & 7)` bajtu `x / 8`. Vzniká z dlaždic (sloupec × 16, řádek × 12, po řádcích,
barva 0 průhledná); obarvení pod hladinou (`| 0x40`) bit 7 nemění a za hry se nemění vůbec. Sonda `113b:1457`
čte jednu VGA rovinu stránky (bajt `y * 0x60 + x / 4`, rovina `x & 3`) a 10 bodů kolem vrtulníku. U horního
a dolního okraje sahá až 20 řádků mimo stránku do sousední paměti VGA. V golden replayích k tomu došlo
281 tisíckrát, ale nikdy tam nebyl pevný pixel, proto jádro bere všechno mimo stránku jako volné.
`GoldenReplayTest` ověřuje masku proti stránce pozadí portu ve všech 81 levelech a hlídá i čtení mimo stránku.

## Level (detail, rozšiřuje fázi 1)

Všechna slova jsou v JSON **surová** (signed 16 bit, souřadnice v prostoru Amigy). Přepočty dělá loader `113b:3976`
a v portu je musí udělat jádro stejnou aritmetikou:
- `113b:3d4d`: `y - (y >> 2)` = **Y × 3/4** (Amiga PAL 256 řádků → 192 na PC), `SAR` = zaokrouhlení dolů.
- `113b:3d5a`: `x + (x >> 1)` = × 3/2.
- `<< 5` převod pixelů na fixed-point 1/32 px, `>> 5` zpět.

**Plošiny (seznam A)**, 8 slov: `w0 w1 y w3 w4 x number w7`. Loader kopíruje w0-w6 do paralelních polí
`290d/2921/2935/2949/295d/2971/2985` (max 10 plošin), w7 přeskočí. Konec = záporné w0.
Pozice pasažérů a objektů se počítají relativně k `x` (`2971`) a `y` (`2935`) plošiny.

**Pasažéři (seznam B)**, 3 slova: `typ, plošina, ukazatel trasy`. Konec = 0xFFFF, max 16.
Typy (ukazatele na deskriptory v DGROUP): `0x7720`, `0x7848`, `0x77b4`, `0x78dc`. Typ `0x78dc` stojí od začátku
na plošině (`x - 8`, `y - 11`) a nemá trasu. **Trasa** = dvojice `(plošina, hodnota)` ukončené 0xFFFF.
Hodnoty 100-600 budou nejspíš jízdné nebo čas, ověří fáze 3.

**Objekty (seznam C)**, typ určuje délku záznamu a obslužnou rutinu v 113b (max 5 objektů):

| typ | slov po typu | rutina | inicializace (113b:3b21) |
|---|---|---|---|
| `0x76e4` | 2 | `2a87` | pozice z plošiny w1 (`x-16`, `y-24`), w2 → `2cd5` |
| `0x7630` | 3 | `2379` | w1 přeskočeno, w2 × 3/2 → `2cdf`, w3 → `2ce9`, `2d57 = 1` |
| `0x76a8` | 4 | `295b` | x = w1 << 5, y = (w2 << 5) × 3/4 − 0x2c0 |
| jiný (`0x766c`) | 3 | `25b1` | pozice z plošiny w1 (`x-16`, `y-22`), w3 → `2ce9 = (w3 >> 2) − w3`, `2d57 = 0` |

Podle popisků prvních levelů (představují postupně jednotlivé nepřátele) to budou dinosaurus, pterodaktyl,
triceratops a podobně. Konkrétní přiřazení určí fáze 3.

**Ostatní pole záznamu**: `+08` (porovnává se s `27cc` při kreslení), `+0a` popisek levelu (0xFE = formátovací
kód, 0x0D = nový řádek), `+0c` příznak (zapíná `113b:3cde` v každém snímku), `+15` počáteční hladina vody,
`+17` rychlost vody (přičítá se každý druhý snímek, `113b:2d1c`), `+19` zatím nepoužitý bajt.

## Sprity (přehled)

| Čísla | Obsah |
|---|---|
| 0-~173 | dlaždice (16×12, 16×10, 16×14, 16×16 …) |
| 174-177 | pruhy 255×7 + 65×7 (= 320 px, šířka je jen bajt), pravděpodobně stavový řádek |
| 214-216 | velké: logo UGH! 127×56, vrtulník 77×50 ×2 |
| dál | stromy, pytel peněz, pterodaktylové, bubliny pasažérů (cíl, otazník, peníze …), ovoce, jeskynní lidé (animace), triceratopsové, font |

## Zbývá na později

- Dekódování ADLX (hudba a efekty), zatím jen surové bloky.
- Kompilované sprity v kódu (`113b:2db9–3961`): až se zjistí, co kreslí.
- Výchozí tabulka nejlepších výsledků v DGROUP a formát `UGH!.HI` patří do jádra hry, ne do extraktoru.
