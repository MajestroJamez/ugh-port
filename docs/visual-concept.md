# Vizuální směr „Pravěké dioráma“ (krok 11, útes ve 3D krok 15)

Level je dioráma: útes rozříznutý přesně v rovině hry. Řez je přední stěna skály, za ním jeskyně do hloubky, vepředu
nic - kamera kouká do řezu jako do vitríny. Hratelnost zůstává 2D a pixelově přesná podle kolizní masky.

## Tvar (krok 15)

Skála je jedno pole hustoty ve 3D (`FUghRockField`, mřížka středů pixelů a vrstev do hloubky), jeho povrch síť
„surface nets“ (`UghSurfaceNets`) s normálami z pole - hladká skála bez schodů po pixelech:

- **Deska hry** (hloubka -20 .. 20 jednotek): pole je vzdálenost od hranice masky, hrana leží přesně na hranicích
  pixelů (dva sousední pixely různého druhu jsou od ní oba půl pixelu). Síť uřízne jen rohy pixelových schodů (nejvýš
  půl pixelu), střed každého pixelu zůstane na své straně - test `Ugh.Rock` řeže síť v rovině hry (a pixel před ní
  a za ní) a porovná ji s maskou ve všech levelech (naměřeno nejvýš 0,12 px od hranic pixelů).
- **Čelo skály** před deskou: zaoblené hrany (čtvrtkruh 2,5 px), mírně hrbolaté, uprostřed širokých skal vypouklé
  (nejvýš 4 px před deskou; bubliny cestujících jsou kartičky ještě před ním).
- **Jeskyně za deskou**: obrys se rozmaže, stěny a stropy čím hlouběji, tím víc lezou do jeskyně (převisy, až 3 px)
  a jsou drsné (šum roste s hloubkou), podlahy zůstávají ve výšce masky (stojí na nich dekorace). Zadní stěna daleko
  (30-78 px, v průměru 46), hlubší za tmavými dírami kresby. Krápníky pod rovnými stropy s místem pod sebou a
  spadané kameny u paty stěn a občas na podlaze (ne na plošinách) - vše až za deskou, nikdy v ní (`UghRockFeatures`).
- **Za okraji obrazovky** útes pokračuje (48 px do stran, 24 nahoru a dolů): pixely okraje pokračují ven a skála se
  postupně zavírá (s šumem), na okraji mřížky úplně; voda pokračuje přes celou šířku. Neviditelný „plášť“ kolem
  mřížky jen vrhá stín útesu, který pokračuje dál (jinak by slunce svítilo přes okraj mřížky pruhy na zadní stěnu).
  Dřevěná krabice zmizela.
- **Cedule s čísly plošin** (dlaždice 85-90 kresby) jsou kartičky s původním spritem těsně za deskou: číslo cílové
  plošiny je herní informace (bez textur je ukáže kresba sama).
- **Postavy**: zatím tvary z plastelíny (vrtulník kvádr + rotor, cestující válec, nepřítel koule, bonus kužel).
  Skutečné modely přijdou později. Bublina cestujícího je kartička s původním spritem (číslo cílové plošiny je
  herní informace).

## Barvy a materiály

- Skála (`M_UghCliff`, kód `Source/UghEditor/Shaders/UghCliff.hlsl`): PBR textury z kroku 14 promítnuté ze tří os
  (triplanár: nic se nenatahuje), míchané podle toho, kam plocha míří, a podle původní kresby (změkčené, UV = obrazovka):
  tráva `grass004` na plochách nahoru, hlína `red_laterite_soil_stones` na svazích, mech `moss002` kde je kresba
  zelená a na hlubokých podlahách, jinak skála - teplá vrstevnatá `cliff_side` kde je kresba teplá (její skála),
  šedá `rock_face_03` kde je šedá a na zadní stěně (tmavší). Kde se vrstvy potkávají, vyhraje ta s vyšším reliéfem
  (výšková mapa). Kresba skálu trochu tónuje, ať má každý level své barvy; barvy vrcholů nesou otevřenost (AO
  ve škvírách) a hloubku. Bez textur (repo bez assetů) má skála barvy kresby jako dřív (`M_UghRock`).
- `M_UghPbr` má zapojenou výškovou mapu (parallax, `BumpOffset`).
- Plastelína: matná (drsnost 0,7), sytější barvy než originál; vrtulníky oranžový a tyrkysový.
- Voda: průsvitná, tmavě modrozelená.

## Světlo a atmosféra

- Večer: teplé slunce zepředu shora zleva (45°, 8 lux; stíny skal padají na zadní stěnu, takže je hloubka čitelná),
  obloha HDRI `sky_belfast_sunset` na kopuli kolem světa, kterou snímá sky light (osvětlení i odrazy), Lumen GI
  s hardwarovým ray tracingem, pevná expozice (EV100 2) - všechny levely stejně světlé. Bez HDRI atmosféra enginu.
- Palmy byly šedé, protože materiály modelů (instance glTF materiálů enginu) nepovolují Nanite a hra kreslí místo
  nich výchozí materiál: import je teď přepojí na kopie v `Content/Imported/_Masters`, které Nanite povolují.
- Ohniště na nejdelší suché římse bez plošiny (jen dekorace, do hry nezasahuje): dvě polena, plamen a blikající
  oranžové bodové světlo, které osvětlí jeskyni.
- Mlha: nízká přízemní (exponential height fog) s objemovou mlhou, ať je hloubka čitelná.
- Vítr (krok 12): level s větrem je bouřka - slabší chladné slunce, tmavá zamračená obloha (`sky_kloofendal_cloudy`
  ztmavená), hustá šedá mlha; plamen ohniště se kloní po větru a víc bliká. Kapky deště jsou plastelínové čárky
  (4 x 1 px) šikmo po větru, na pozicích z logiky.
- Menu: za ním ztlumené dioráma levelu, který by menu spustilo (po heslu ten level).

## Kamera

Pevná, úzký objektiv (30°), celý řez v záběru, mírně shora (-4°), aby byly vidět horní plochy plošin.

## Technika

- Materiály generuje editorový commandlet `UghMakeAssets` (C++, `5_remake/game/Source/UghEditor/`) do
  `Content/Generated/` při každém `build.ps1`: žádné binární assety v gitu.
- Geometrie útesu: pole a jeho síť při načtení levelu (asi 0,2 s, 300-400 tisíc trojúhelníků), za běhu postavená
  jako statická síť enginu (`FUghRockMesh::ToStaticMesh`), takže se její stíny (VSM) a ray tracing cachují -
  `ProceduralMeshComponent` se kreslil do stínů každý snímek (polovina času snímku). Nanite ne: síť vzniká za běhu
  a Nanite se staví jen v editoru.
- Výkon (Radeon 890M, 1280 x 720 bez okna, FSR 67 %): jednotlivé snímky 50-57 fps (krok 14: asi 75), v dlouhém
  `levels.ps1` se notebook zahřeje a klesne to na medián 33 (krok 14: 45). Snímky logují fps
  (`UGH shot: ... fps`).
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
