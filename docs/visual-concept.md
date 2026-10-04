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

## 3D assety (krok 14)

Knihovna pro kroky 15-23 (hra plně ve 3D): jen volně použitelné assety (CC0, stažitelné bez účtu), jen lokálně
v `assets/3d/` (necommitují se, celkem do 100 GB, dnes 0,57 GB). Seznam se zdrojem, autorem, licencí a cestou je
`5_remake/game/Assets.json`; stahuje `fetch-assets.ps1`, importuje commandlet `UghImportAssets` (`build.ps1`) do
`Content/Imported/<id>`. Bez assetů hra ukáže tvary z plastelíny a zaloguje to.

| Zdroj | Licence | Co odtud bereme | Jak se stahuje |
|---|---|---|---|
| [Poly Haven](https://polyhaven.com) | CC0 | textury skal a hlíny (2k: barva, normála DirectX, AO/drsnost/kov, výška 16 bit), modely (glTF 2k), oblohy HDRI (4k) | veřejné API `api.polyhaven.com/files/<id>`, kontrola MD5 z API |
| [ambientCG](https://ambientcg.com) | CC0 | textury trávy a mechu (2k JPG) | přímý zip `ambientcg.com/get?file=...`, SHA-256 v manifestu |
| [Kenney](https://kenney.nl) Nature Kit | CC0 | ohniště (kameny, polena; low poly FBX) | přímý zip, SHA-256 v manifestu |
| [OpenGameArt](https://opengameart.org/content/free-palm-treez-v3), Nobiax | CC0 | palmy (rovná, ohnutá) | přímý 7z (rozbalí `tar.exe` Windows), SHA-256; `Blender/palm.py` z OBJ + TGA udělá glTF s vyříznutými listy |

Quaternius (CC0) zatím ne: balíky jsou jen přes Google Drive / itch.io, ne přímým odkazem.

První várka (útes v džungli doby kamenné):

- **textury** (instance `MI_<id>` materiálu `M_UghPbr`): skály `cliff_side` (vrstevnatý útes, teplý), `rock_face_03`
  (šedá skalní stěna), `rock_wall_02` (rozpukaná), `mossy_rock` (mech na skále), `lichen_rock` (lišejník); hlína
  `red_laterite_soil_stones` (tropická červená hlína); kůra `palm_bark`; tráva `grass004`, mech `moss002`,
- **oblohy**: `sky_belfast_sunset` (večer s mraky - nálada diorámatu), `sky_kloofendal_cloudy` (den s mraky),
- **modely** (Nanite): balvany `boulder_01`, `namaqualand_boulder_02`, kameny `rock_moss_set_01`, `stone_01`, palmy
  `palm`, strom džungle `island_tree_02`, kapradina `fern_02`, keř `shrub_02`, rostliny džungle `pachira_aquatica_01`,
  `calathea_orbifolia_01`, tráva `grass_medium_01`, kmen `dead_tree_trunk`, ohniště `campfire`.

Ve hře zatím jen důkaz cesty: v každém levelu palma (a kde se vejdou, jeden až dva kameny) na suché římse s místem
nad sebou, mimo plošiny (kde jinde místo není, vysoká palma na římse plošiny - koruna nad cedulí s číslem), za deskou
hry, takže nikdy nezakryje postavu; stejný level = stejné dekorace (`UghDecorations`, test `Ugh.Scenery`).
