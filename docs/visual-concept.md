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
- **Vrtulník** (krok 16): pravěký šlapací vrtulník podle spritu originálu - klec z bambusu svázaného provazem na
  podlaze z kmenů, vzadu a po stranách proplétané proutí, dva pařezy s koženými polštáři, řídítka z kosti na kůlu,
  klika s kostěnými rameny a kamennými pedály, kožené prapory nahoře a dole vpředu a kly na předních rozích; nahoře
  rotor z kostí s velkými listy přivázanými na kamenném náboji (druhý hráč keřovitější, jako v originálu). Tělo
  vyplňuje tělo vrtulníku logiky (`COPTER_BODY_*`, 22 x 20 px) a do hloubky zůstává v ±45 jednotkách, tedy za čelem
  skály (-60) i bublinami; rotor (poloměr 1,3 m) přesahuje tělo do stran jen o 2 px; do hloubky
  opisuje kruh, před čelem skály ale jen nad vzduchem těla (čelo stojí jen na pevných pixelech). Barvy týmu jsou barvy
  kůže: hráč 1 rezavě oranžová, hráč 2 tyrkysová. Rotor se točí plynule tak rychle, jak rychle se mění jeho sprity
  v logice (6 spritů na otáčku), klika 3x pomaleji; pilot šlape přesně s klikou (akce `pedal` držená v úhlu kliky).
- **Pilot a cestující** (krok 16): jeskynní muž - zavalitý, kreslený, asi 1,15 m (sprity originálu jsou 10 px vysoké
  a 16 px široké), velká hlava s obočím, nosem a očima, kůže z leopardí kožešiny přes rameno, vlasy krátké nebo
  dlouhé a vousy jako části, které hra ukáže podle vzhledu, barvy vlasů, kožešiny a kůže podle vzhledu. Riggovaný
  (kostra s pánví, páteří, hrudí, krkem, hlavou a končetinami) s akcemi `idle`, `sit`, `pedal`, `hang`.
  Cestující v kabině sedí za pilotem napravo a výš, takže je ve vrtulníku vidět; vzhled podle `cargo_look` logiky:
  1 mladík s krátkými vlasy, 2 žena s dlouhými rudými vlasy, 3 stařec s šedými vlasy a vousy. Visící cestující
  (vzhled 4) je kámen s očima (`standingPassenger` originálu) v provazové smyčce pod podlahou, houpe se proti
  směru letu.
- **Cestující** (krok 17): jeskynní lidé podle vzhledu druhu (mladík, žena s dlouhými vlasy, stařec; výška podle
  spritu, vysoká žena štíhlejší) dělají, co říká sprite logiky: chodí (krok se drží snímků animace spritu), mávají na
  vrtulník oběma rukama, čekají, vycházejí ze dveří a jdou do nich (zezadu z hloubky 60 jednotek), ve vodě šlapou vodu
  (hladina těsně pod bradou), plavou s hlavou nad vodou a padají nebo tonou s rukama nahoře. Postava jdoucí do strany
  se natáčí o 20° ke kameře. Stojící cestující je kámen s očima, padající kámen se kutálí. Bublina cestujícího zůstává
  kartička s původním spritem (číslo cílové plošiny je herní informace) před čelem skály.
- **Nepřátelé** (krok 17): pterodaktyl (hnědá šupinatá kůže, oranžový zobák a hřebínek, kožená křídla; dva mávnutí
  a plachtění na smyčku 12 spritů letu, zasažený padá a točí se), v letu naklopený zády 35° ke kameře a 1,3× větší
  (křídla jsou v hloubce, z boku by byla vidět jen hranou); triceratops (olivově hnědý, oranžový límec se skvrnami,
  tři rohy, zamračené oči; chůze, hrabání kopytem při čekání, výpad se skloněnou hlavou, třepání hlavou, omráčený leží
  a nad hlavou krouží hvězdičky); foukač je kulatá bradavičnatá šelma s chobotem a tvářemi, které se při nádechu
  nafouknou a pak vyfouknou (smyčka 10 spritů, první 3 nádech), omráčený se schoulí pod hvězdičkami; strom s tváří
  (kůra `palm_bark`, koruna z listnatých bochníků a lístků, červené plody, mrká v posledním snímku houpání, setřesený
  zavře oči a třese se). Všichni se dívají směrem pohybu (o 20-35° ke kameře), velcí jako svůj sprite.
- **Bonusové předměty** (krok 17): devět plodů podle spritů originálu (jablko, meloun, jahoda, kiwi, kokos, hruška,
  půlka kokosu, třešně, banán) a kamenná tabulka se zeleným X (násobitel), asi 1 m, pomalu se otáčejí.
- Bez modelů (repo bez assetů) zůstávají tvary z plastelíny (cestující válec, nepřítel koule, bonus kužel).

## Barvy a materiály

- Skála (`M_UghCliff`, kód `Source/UghEditor/Shaders/UghCliff.hlsl`): PBR textury z kroku 14 promítnuté ze tří os
  (triplanár: nic se nenatahuje), míchané podle toho, kam plocha míří, a podle původní kresby (změkčené, UV = obrazovka):
  tráva `grass004` na plochách nahoru, hlína `red_laterite_soil_stones` na svazích, mech `moss002` kde je kresba
  zelená a na hlubokých podlahách, jinak skála - teplá vrstevnatá `cliff_side` kde je kresba teplá (její skála),
  šedá `rock_face_03` kde je šedá a na zadní stěně (tmavší). Kde se vrstvy potkávají, vyhraje ta s vyšším reliéfem
  (výšková mapa). Kresba skálu trochu tónuje, ať má každý level své barvy; barvy vrcholů nesou otevřenost (AO
  ve škvírách) a hloubku. Bez textur (repo bez assetů) má skála barvy kresby jako dřív (`M_UghRock`).
- `M_UghPbr` má zapojenou výškovou mapu (parallax, `BumpOffset`).
- Plastelína: matná (drsnost 0,7), sytější barvy než originál; vrtulníky bez modelů oranžový a tyrkysový.
- Vrtulník a jeskynní muž (krok 16): textury z kroku 14 (kůra `palm_bark` na kmeny a pařezy, `rock_face_03` na
  kámen) a procedurální textury spočítané skriptem (bambus s kolénky, proutí s průhlednými dírami, kůže, kost,
  provaz, list s žilkami; kůže člověka, vlasy, leopardí kožešina). Barvy, které hra mění (kůže vrtulníku, listy,
  vlasy, kožešina), jsou faktory základní barvy glTF nad světlou texturou.
- Voda: průsvitná, tmavě modrozelená.

## Světlo a atmosféra

- Večer: teplé slunce zepředu shora zleva (45°, 8 lux; stíny skal padají na zadní stěnu, takže je hloubka čitelná),
  obloha HDRI `sky_belfast_sunset` na kopuli kolem světa, kterou snímá sky light (osvětlení i odrazy), Lumen GI
  s hardwarovým ray tracingem, pevná expozice (EV100 2) - všechny levely stejně světlé. Bez HDRI atmosféra enginu.
- Palmy byly šedé, protože materiály modelů (instance glTF materiálů enginu) nepovolují Nanite a hra kreslí místo
  nich výchozí materiál: import je teď přepojí na kopie v `Content/Imported/_Masters`, které Nanite povolují.
- Ohně (krok 18): až tři na nejdelších suchých římsách (napřed bez plošiny), za deskou hry: kamenný kruh a polena
  (Kenney, v tmavší plastelíně), plamen ze tří zkřížených kartiček (materiál `M_UghFire`, `UghFlame.hlsl`: jazyky ohně
  olizují vzhůru stoupajícím šumem, jiskry, každá kartička jinak) a blikající oranžové bodové světlo každého (Lumen),
  každý oheň jinak; zhasne, když voda vystoupí nad jeho římsu.
- Mlha: nízká přízemní (exponential height fog) s objemovou mlhou, ať je hloubka čitelná.
- Vítr (krok 12): level s větrem je bouřka - slabší chladné slunce, tmavá zamračená obloha (`sky_kloofendal_cloudy`
  ztmavená), hustá šedá mlha; plameny ohňů se kloní po větru a víc blikají. Kapky deště jsou plastelínové čárky
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
  (`UGH shot: ... fps`), `levels.ps1` vypíše na konci medián. Krok 18 (asi 2000 dekorací na level, z toho 1000-2000
  trsů trávy a květin): jednotlivé snímky 49-53 fps, `levels.ps1` medián 40 (nejpomalejší 13 při zahřátí uprostřed
  běhu, pak zase 40+); tráva a květiny bez stínů, maskované listy bez ray tracingu, instancované (Nanite).
  Krok 18b (Electric Dreams, Substrate, 40-60 naskenovaných útesů a kořenů na level): jednotlivé snímky 42-46 fps,
  `levels.ps1` medián 43, nejpomalejší 27; první spuštění po kopii staví textury vzorku (~1 min) a shadery Substrate.
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
- **krok 18** (Poly Haven, Nanite): tráva `grass_medium_02`, květiny `flower_gazania` (oranžové), `flower_empodium`
  (žluté), `periwinkle_plant` (růžové), rostlina džungle s červenými květy `anthurium_botany_01`, nízký keř
  `shrub_04`, pařezy `tree_stump_01`; z Blenderu (`generated`): kosti a lebky `bones`, totemy `totem`, chýše `hut`,
  liány `vines`. Nepoužité zatím: `island_tree_02` (koruna se do jeskyně nevejde), `pachira_aquatica_01` (kmen a listy
  jsou zvláštní sítě), `dead_tree_trunk` (3 m dlouhý), textury `rock_wall_02`, `mossy_rock`, `lichen_rock`.

## Příroda a dekorace (krok 18)

Pravěká džungle na římsách: na horních plochách skály louky, palmy, keře, kapradiny, rostliny džungle, kameny,
pařezy, kosti, totemy a chýše kmene, ohně, ze stropů a převisů visí liány a po zadní stěně jejich závěsy. Rozmístění (`UghDecorations::Plan`) je
z masky a pole skály (`FUghRockField`: každá dekorace stojí na podlaze ve své hloubce, liána visí ze stropu, přední
polovina krabice je ve vzduchu, zadní se smí opřít o zadní stěnu jeskyně), deterministicky (level_id), napřed velké:

- **ohně**: až 3 na nejdelších suchých římsách (napřed bez plošiny), 40 px od sebe,
- **palmy**: až 4 nejvyšší, které se vejdou (16-48 px), 30 px od sebe; kde se žádná nevejde, menší (10-16 px),
- **totemy** (dva, vyřezávané malované tváře s křídly / tyč s lebkou triceratopse, kostmi a pery) a **chýše** (stan
  z kůží na tyčích / kupole ze slámy s kly mamuta) tam, kde je místo,
- **podél každé římsy** každé 2-4 px keř, kapradina, rostlina džungle, kámen (i mechový), pařez nebo kosti (dlouhá
  kost, hromádka kostí, lebka medvěda, lebka triceratopse, hrudní koš velké šelmy),
- **louky**: trs trávy každých 0,8 px v řadách po 15 jednotkách do hloubky (od nejbližší povolené hloubky 150 dál),
  v ostrůvcích květiny (šum podél říms),
- **liány** ze stropů masky (asi každý třetí pixel stropu s místem pod sebou, 6-44 px dlouhé, natočené ke kameře)
  a **závěsy lián po zadní stěně jeskyně**: z vršku každého třetího sloupce vzduchu (strop nebo horní okraj
  obrazovky - útes pokračuje) v ostrůvcích (šum), kusy 10-30 px pod sebou (každý s pravděpodobností 0,7 pokračuje),
  opřené o nejbližší místo stěny za sebou, dolů nejvýš do 85 % místa pod stropem.

Nic nezakrývá postavy, plošiny, cedule ani bubliny (`UghDecorations`, test `Ugh.Scenery` pro všech 150 levelů):
nic není na obrazovce blíž než `SlabFront` (25 jednotek za rovinou hry: deska hry, cedule 23), nic vyššího než
pokryv země (5 px: tráva, květiny, kosti, malé kameny) blíž než `FigureReach` (80: tělo vrtulníku 45, triceratops,
foukač a strom s tváří 70), nic vyššího než 16 px ani liána blíž než `SweepReach` (180: rotor 130, křídla letce 170);
kde přistává vrtulník na plošině, tam kde je jeho tělo (11 px do stran, 20 nahoru) nic blíž než 80, kde se točí
rotor (13 px do stran, 16-24 px nad plošinou) nic blíž než 180. Listnaté rostliny se smí prolínat (od sebe mají
jen prostředky), tráva roste i u kmene palmy, do ohně nezasahuje nic.

Kreslení (`AUghScenery`): jedna instancovaná komponenta na síť (Nanite), model vložený do své krabice; tráva
a květiny nevrhají stíny, listy (maskované) nevidí ray tracing; bez modelů plastelínové tvary. Průsvitné materiály
glTF (tráva, květiny: alphaMode BLEND) import přepne na vyříznuté (Nanite průsvitnost nekreslí).

## Fotorealistický svět z Electric Dreams (krok 18b)

Jan: vzhled připomínal hru z GameMakeru (plastelína, CC0 assety, cihly kresby). Zdroj fotorealismu je Epicův vzorek
**Electric Dreams Environment** (UE 5.8, `C:\Users\Ja079591\IdeaProjects\Unreal Projects\ElectricDreamsEnv`, ~56 GB;
Megascans a vlastní assety Epicu, licence jen pro projekty Unreal Engine - nikdy v gitu). Vzorek se jen čte:
`electric-dreams.ps1` (commandlet `UghCopyElectricDreams`) zkopíruje vybrané assety se závislostmi do
`Content/External/ElectricDreams` (gitignore, 257 balíčků, 3,1 GB), `[CoreRedirects]` v `DefaultEngine.ini` přesměrují
jejich odkazy `/Game/<cesta>` do kopie. Materiály vzorku jsou Substrate (bez něj černé) a textury virtuální (bez
virtuálních textur projektu černé): projekt má `r.Substrate=True` a `r.VirtualTextures=False`.

### Inventura vzorku (velikosti zdrojových složek)

| Druh | Ve vzorku (`Content/...`) | Velikost | Vybráno |
|---|---|---|---|
| útesy, skalní stěny (Nanite) | `Megascans/3D_Assets`: `HugeSandstoneCliff` (8 sítí), `MassiveSandstoneCliff` (6), `MossyRockFace` (3), `QuarryCliff`, `HugeMossyForestCliff`, `ForestRockFormation`, `CanyonSandstoneRidge`, `HugeNordicCoastalCliff`; `SmartAssets/AssetMeshes` `SM_Cliff_01..19`; `Meshes/_GENERATED/RockFormation` | 1,7 GB, 1,0, 0,9, 1,0, 0,09, 0,7, 0,6, 0,3; 4,1; 3,2 | `HugeSandstoneCliff_01`, `_03`, `MassiveSandstoneCliff_02` (zadní stěna) |
| balvany, kameny | `SandstoneBoulder`, `MossyForestBoulder`, `MossyForestRock`, `LichenedForestBoulder`, `MossyRocks`, `NordicBoulder`, `SmallStonesPack`, `TundraMossyBoulder` | 0,6, 1,0, 0,4, 0,06, 0,7, 0,2, 0,3, 0,2 GB | `SmallStonesPack_01/03/05`, `MossyForestRock_01`, `LichenedForestBoulder_01` (kameny na římsách) |
| kořeny | `Custom/RootsTest` (`SM_Roots_01..14`, `SM_ErosionRoots_01..04`), `Megascans/3D_Assets/ForestRoots`, `MossyForestRoots` | 3,3 GB (z toho 2,2 erozní kořeny), 0,6, 0,5 | `SM_Roots_01..05` (visí ze stropů jeskyně) |
| pařezy, kmeny | `TreeStump`, `RottenTreeStump`, `BrokenStump`, `BirchTreeStump`, `FallenTreeAssembly`, `OldFallenTree`, `DeadTree` | 0,2, 0,3, 0,4, 0,3, 0,3, 0,3, 0,3 GB | `TreeStump_01`, `RottenTreeStump_02` |
| palmy | `Megascans/3D_Plants`: `AlexandraPalm`, `ArecaPalm`, `CatPalm`, `CanePalm`, `FanPalm`, `BroadleafLadyPalm`, `ZamiaPalm` | 23-48 MB každá | `AlexandraPalm_01..03`, `FanPalm_01`, `ArecaPalm_03`, `CatPalm_01` |
| rostliny s velkými listy | `Taro`, `BirdOfParadise`, `CastorOilPlant`, `ArrowheadPlant`, `PinkCordyline`, `VariegatedCroton`, `JungleGinger`, `MoneyPlant`, `DragonTree`, `JanetCraigDracaena` | 22-59 MB | `Taro_02/05/06/10`, `ArrowheadPlant_04`, `PinkCordyline_02`, `VariegatedCroton_01`, `BirdOfParadise_01` (rostliny), `ArrowheadPlant_01/02/05`, `VariegatedCroton_02`, `CastorOilPlant_01` (keře) |
| kapradiny | `Fern`, `BeechFern`, `SilverLadyFern`, `BostonFern` | 29, 195, 57, 12 MB | `Fern_01/02/05`, `BeechFern_04`, `SilverLadyFern_01`, `BostonFern_02` |
| tráva, květiny, pokryv | `KikuyuGrass`, `GroundCover`, `CloverVarieties`, `WhiteWindflower`, `RedLachenalia`, `Amaryllis`, `Periwinkle`, `LilyOfTheValley`, `CustomMoss`, `DeadLeaves` | 3-52 MB | `KikuyuGrass_01/03/05/06`; `WhiteWindflower_01/03`, `RedLachenalia_06`, `Amaryllis_03`, `GroundCover_07` |
| liány | `Megascans/3D_Plants/Ivy` (25 sítí), `EnglishIvy`, `Custom/IvyTest` (`SM_HangingVine_01..11`, `SM_CliffVine_01..09`) | 109, 16, 1 870 MB | `Ivy_11/13/15/17/21/23`, `HangingVine_09/10` (ze stropů), `Ivy_09/12/16/18/19/22` (závěsy po zadní stěně) |
| povrchy (dlaždicové) | `Megascans/Surfaces`: `BeachCliff`, `JungleGround`, `MossyGrass`, `NordicMoss`, `MossyRockyGround`, `ButtressRoot`, `IcelandicQuarryRock`; `SmartAssets/TileableTextures` `T_Rock_01..03`, `T_Moss_02`, `T_Lichen_01` | 0,9 GB; 0,2 GB | vrstvy skály: `T_Rock_03` (teplá skála), `T_Rock_01` (šedá), `MossyGrass`, `NordicMoss`, `JungleGround` |

Náhledy pro výběr byly miniatury uložené v hlavičkách `.uasset` (bez spuštění editoru vzorku). Odmítnuto: textury
naskenovaných skal (`BeachCliff`) se na velké skále zjevně opakují; skenované útesy jsou zezadu otevřené (jen čelem ke
kameře); balvany a kořeny vsazené do čela skály v rovině hry vypadaly jako nalepené oblázky (pevné oblasti masky jsou
na ně tenké) - čelo skály zůstává sítí z pole s naskenovaným materiálem.

### Jak se to používá

- **Zadní stěna jeskyně** (`UghRockDressing`, `AUghCliffDressing`): velké naskenované pískovcové útesy (poloměr
  40-65 px, mřížka 24 px i za okraji obrazovky, natočené ke kameře ±15°, překrývají se) vsazené do zadní stěny, ven
  45-70 % hloubky modelu; nikdy blíž než 250 jednotek (dosah rotoru a křídel 180) ani blíž než prostředek dekorace
  před nimi (kromě pokryvu a lián). Kořeny visí ze stropů (každý druhý sloupec po 5 px pod stropem s 14 px místa) před
  útesem, který tam je. Test `Ugh.Dressing` (150 levelů: aspoň 10 útesů, nejméně 23, deterministicky, za dosahem
  postav a za dekoracemi).
- **Čelo skály** (`M_UghCliff`): stejný triplanár, vrstvy z dlaždicových povrchů vzorku (`T_Rock_03` teplá, `T_Rock_01`
  šedá, `MossyGrass`, `NordicMoss`, `JungleGround`), reliéf podle kanálů `HeightMask` vrstvy, velké skvrny světlejší
  a tmavší (reliéf 8x větší) proti opakování, kresba změkčená na 8 px a tónuje jen trochu, teplá skála do pískovce.
  Hrana v rovině hry je dál maska (test `Ugh.Rock`).
- **Dekorace**: druhy kroku 18 dostanou modely vzorku (`UghElectricDreams.h`: palmy s velkými listy, rostliny džungle,
  kapradiny, keře, tráva, květiny, kameny, pařezy, liány břečťanu); kosti, totemy, chýše a ohně zůstávají z Blenderu
  a Kenneyho. Pravidla chráněných objemů beze změny (`Ugh.Scenery`).
- **Světlo a post**: expozice EV100 1,5 (o půl clony světlejší), jemný bloom 0,35, teplé vyvážení bílé 6100 K,
  sytost a kontrast +8 % / +6 %, vinětace 0,3; Lumen a mlha jako dřív.
- Bez kopie vzorku hra ukáže CC0 assety kroku 14-18 (a jednou to zaloguje).
