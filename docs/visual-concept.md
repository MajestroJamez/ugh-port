# Vizuální směr „Pravěké dioráma“ (krok 11)

Level je dioráma: útes rozříznutý přesně v rovině hry. Řez je přední stěna skály, za ním jeskyně do hloubky, vepředu
nic - kamera kouká do řezu jako do vitríny. Hratelnost zůstává 2D a pixelově přesná podle kolizní masky.

## Tvar

- **Řez** (rovina hry): plné pixely kolizní masky, vytažené dozadu do skály. Hrany po pixelech jsou vidět jako
  vrstvy horniny (schody 1 px = 10 cm) - masku nikdo nehladí, aby hra a obraz nikdy nenesouhlasily.
- **Jeskyně za řezem**: zadní stěna v hloubce s hrbolatým povrchem (šum), obarvená kresbou levelu jako řez.
- **Rám**: dřevěná krabice diorámatu kolem obrazovky originálu (320 x 192 px), aby okraje nebyly „konec světa“.
- **Postavy**: zatím tvary z plastelíny (vrtulník kvádr + rotor, cestující válec, nepřítel koule, bonus kužel).
  Skutečné modely přijdou později. Bublina cestujícího je kartička s původním spritem (číslo cílové plošiny je
  herní informace).

## Barvy a materiály

- Barvy skály a jeskyně bere materiál z původní kresby levelu (dlaždice z `assets/`, složené za běhu do textury
  320 x 192): řez i stěny jeskyně mají barvy originálu, změkčené a s hrbolatostí hlíny. Kresba se necommituje.
- Plastelína: matná (drsnost 0,7), sytější barvy než originál; vrtulníky oranžový a tyrkysový.
- Voda: průsvitná, tmavě modrozelená.

## Světlo a atmosféra

- Večer: nízké teplé slunce zepředu shora (bez zabarvení atmosférou, ať kameny zůstanou modrošedé), obloha jako
  světlo okolí, expozice o stupeň níž, Lumen GI (na RTX s hardwarovým ray tracingem).
- Ohniště na nejdelší suché římse bez plošiny (jen dekorace, do hry nezasahuje): dvě polena, plamen a blikající
  oranžové bodové světlo, které osvětlí jeskyni.
- Mlha: nízká přízemní (exponential height fog) s objemovou mlhou, ať je hloubka čitelná.
- Vítr (krok 12): level s větrem je bouřka - slabší chladné slunce, slabší obloha, hustá šedá mlha; plamen ohniště
  se kloní po větru a víc bliká. Kapky deště jsou plastelínové čárky (4 x 1 px) šikmo po větru, na pozicích z logiky.
- Menu: za ním ztlumené dioráma levelu, který by menu spustilo (po heslu ten level).

## Kamera

Pevná, úzký objektiv (30°), celý řez v záběru, mírně shora (-4°), aby byly vidět horní plochy plošin.

## Technika

- Materiály generuje editorový commandlet `UghMakeAssets` (C++, `5_remake/game/Source/UghEditor/`) do
  `Content/Generated/` při každém `build.ps1`: žádné binární assety v gitu.
- Geometrie útesu: `ProceduralMeshComponent` z masky při načtení levelu.
- Balení: `package.ps1` (UAT, `NO_PROXY += ::1`, data z `assets/` vedle hry), PSO cache nahraná autopilotem
  `-UghShot` v zabalené hře (`pso.ps1`), aby se při startu netrhalo.
- Kontrola všech levelů: `levels.ps1` - autopilot spustí každý level obou režimů z menu (heslem), nafotí ho bez okna
  a složí kontaktní archy `Saved\Shots\Levels\levels-1p.png` a `levels-team.png`.
