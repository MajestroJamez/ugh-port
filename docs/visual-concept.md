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
- **Čelo skály** před deskou: zaoblené hrany (čtvrtkruh 2,5 px), v reliéfu (krok 18c: vrstvy pískovce asi 9 px
  vysoké, zvlněné, každá u paty víc venku se zářezem pod sebou, jinde výrazné, jinde skoro žádné; skoro svislé pukliny,
  široké boule, zrno), uprostřed širokých skal vypouklé (0,8-4 px před deskou; bubliny cestujících jsou kartičky ještě
  před ním).
- **Jeskyně za deskou**: obrys se rozmaže, stěny a stropy čím hlouběji, tím víc lezou do jeskyně (převisy, až 3 px)
  a jsou drsné (šum roste s hloubkou), podlahy zůstávají ve výšce masky (stojí na nich dekorace). Zadní stěna daleko
  (30-78 px, v průměru 46), hlubší za tmavými dírami kresby. Krápníky pod rovnými stropy s místem pod sebou a
  spadané kameny u paty stěn a občas na podlaze (ne na plošinách) - vše až za deskou, nikdy v ní (`UghRockFeatures`).
- **Za okraji obrazovky** útes pokračuje (48 px do stran, 24 nahoru a dolů): pixely okraje pokračují ven a skála se
  postupně zavírá (s šumem), na okraji mřížky úplně; kolem útesu je moře (krok 19). Neviditelný „plášť“ kolem
  mřížky jen vrhá stín útesu, který pokračuje dál (jinak by slunce svítilo přes okraj mřížky pruhy na zadní stěnu).
  Dřevěná krabice zmizela.
- **Cedule s čísly plošin** (dlaždice 85-90 kresby) jsou kartičky s původním spritem těsně za deskou: číslo cílové
  plošiny je herní informace (bez textur je ukáže kresba sama).
- **Vrtulník** (krok 16): pravěký šlapací vrtulník podle spritu originálu - klec z bambusu svázaného provazem na
  podlaze z kmenů, vzadu a po stranách proplétané proutí, řídítka z kosti na kůlu, klika s kostěnými rameny
  a kamennými pedály, kožené prapory nahoře a dole vpředu a kly na předních rozích; nahoře
  rotor z kostí s velkými listy přivázanými na kamenném náboji (druhý hráč keřovitější, jako v originálu). Tělo
  vyplňuje tělo vrtulníku logiky (`COPTER_BODY_*`, 22 x 20 px) a do hloubky zůstává v ±45 jednotkách, tedy za čelem
  skály (-60) i bublinami; rotor (poloměr 1,3 m) přesahuje tělo do stran jen o 2 px; do hloubky
  opisuje kruh, před čelem skály ale jen nad vzduchem těla (čelo stojí jen na pevných pixelech). Barvy týmu jsou barvy
  kůže: hráč 1 rezavě oranžová, hráč 2 tyrkysová. Rotor se točí plynule tak rychle, jak rychle se mění jeho sprity
  v logice (6 spritů na otáčku), klika 3x pomaleji; pilot šlape přesně s klikou (akce `pedal` držená v úhlu kliky).
- **Šlapání a sezení** (krok 24b): na první pohled je vidět, že pilot šlape. Pilot sedí z profilu (natočený o 70°
  doleva) na koženém sedátku s opěradlem na bambusových nohách, klika má poloměr 12 cm a na straně ke kameře velké
  kolo s kostěnými paprsky a 18 zuby (poloměr 18 cm). Řetěz z kostěných článků vede z kola nahoru na pastorek se 6 zuby
  na předlohové hřídeli pod střechou (visí na bambusovém závěsu z horního předního trámu), její lucernové kolo točí
  korunové kolo s kolíky na hřídeli rotoru - převod 3:1 jako `FUghRotorSpin::RotorTurnsPerPedal`, takže rotor, hřídel,
  pastorek, řetěz, kolo, pedály i nohy pilota jdou spolu. Čísla v `copter_layout.py` a `UghCopterModel.h`
  (`Chainring`, `Sprocket` z nich odvozené: řetěz v rovině kolmé na osu kliky, předlohová hřídel míří na osu rotoru);
  články jsou instance jedné sítě (`ChainLink`), hra je každý snímek posune po dráze řetězu (`FUghCopterChain`: dvě
  tečny a dva oblouky, jeden zub kola na článek, po otáčce kliky stejně). Cestující má vlastní židli: sedák ze
  štípaných kmenů s koženým polštářem, opěradlo z kostěných žeber pod bambusovou příčkou, područky končící kly.
  Proutí zůstává neprůhledné (krok 24a). Přistání: `shot.ps1 -Land -At 8` (autopilot nechá vrtulníky pomalu klesat).
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
- **Nepřátelé a kámen fotorealisticky** (krok 19h; kde chybí lokální zdroje, zůstávají modely kroku 17):
  - Walker je Janův triceratops ze Sketchfabu (`walker_triceratops.py`): import opravený (kořenová kost FBX má měřítko
    0,2, které Blender dá síti, ale ne kostem - síť ×5 kolem kořene), otočený a 3,1 m dlouhý, síť dvakrát
    subdivovaná s ostrými hranami (rohy, okraj límce, drápy), kůže upečená z jeho textury do olivově hnědé se šupinami
    v prostoru modelu (velké destičky mezi drobnými šupinkami, tmavší spáry; barva, normála, drsnost 2048). Akce z jeho
    vlastních: chůze = walk, čekání = idle, výpad = run, zotavení = attack1 (pohodí rohy), omráčený = začátek die (sesune
    se na břicho, hlava se kymácí).
  - Foukač je Janův T-rex z Fabu (`blower_trex.py`, low poly 28 tis. vrcholů, jeho PBR mapy): kostra 27 kostí
    a automatické váhy Blenderu, spodní čelist jen čelisti; leží a spí na břiše (nohy složené pod tělem jako u ptáka,
    ocas po zemi zatočený dozadu od kamery, hlava na zemi pootočená ke kameře, tlama zavřená, oči zavřené - víčka
    domalovaná do textury), ta póza je jeho klidová. Ve hře natočený doleva o 25° ke kameře zabere 3,1 m (sprite
    3,2 m), nic blíž kameře než 60 cm, ocas sahá za desku hry. Akce: `blow` = hluboký nádech (žebra se vzdouvají,
    hlava se zvedne; první 3 z 10 spritů) a mocné odfrknutí (hlava vyrazí dopředu, žebra splasknou), `stunned` =
    třepe hlavou. Při odfrknutí mu z nozder (kost `nostrils`) vyletí obláček prachu a dechu (`FUghSnort`, materiál
    `M_UghPuff`: čtverečky posouvané GPU, nic za snímek).
  - Strom s tváří (`tree_hornbeam.py`) je starý hrabový pahýl: zavalitý rýhovaný kmen s kořeny a větvemi
    (metaballs), kůra naskenovaná z Electric Dreams (dlaždicová kůra habru promítnutá ze tří os, tmavší v dutinách)
    s vyřezanou tváří (hluboké oční důlky pod obočím s vlhkýma očima, suk nosu, otevřená ústa), koruna ze dvou
    naskenovaných korun mladého habru (větvičky a listy), mezi listy pár planých jablek; víčka z kůry ukáže jen
    mrknutí. Akce jako dřív (`sway` s mrknutím, `shaken`).
  - Kámen (cestující vzhledu 4 i zmenšený na sedadle) je naskenovaný mechem porostlý kámen `MossyForestRock_02`
    (`stone_boulder.py`: z milionu trojúhelníků 40 tisíc, jeho mapy) vtěsnaný do elipsoidu kroku 16, s vlhkýma očima
    zapuštěnýma do mechové tváře pod těžkými víčky z kamene.
  - Pterodaktyl (`pterodactyl.py` znovu): štíhlý pteranodon s hlubokou hrudí, dlouhým zúženým zobákem z rohoviny,
    hřebenem dozadu (červené a okrové pruhy), malýma plazíma očima, blánou křídla od konce prstu ke kotníkům a malou
    přední blánou; kůže a blány upečené (tmavý hřbet, světlé břicho, jemné šupinky a chmýří; blána teplá hnědá,
    světlejší, kde je tenká, s vlákny od prstu k zadnímu okraji a tmavými větvenými žilkami). Při mávání ruka
    zaostává za paží a při zdvihu se křídlo napůl složí. Blány mají ve hře vlastní materiál `M_UghMembrane`
    (dvoustranný „foliage“ s mapami modelu): proti slunci teple prosvítají.
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
  ve škvírách), hloubku, blízkost pod horní hranou masky (tráva a mech přes hranu) a velké skvrny (krok 18c). Od
  kroku 19c je skála šedý vápenec `marble_cliff_03` a šedý kámen `mossy_rock` (viz níže). Bez textur (repo bez
  assetů) má skála barvy kresby jako dřív (`M_UghRock`).
- `M_UghPbr` má zapojenou výškovou mapu (parallax, `BumpOffset`).
- Plastelína: matná (drsnost 0,7), sytější barvy než originál; vrtulníky bez modelů oranžový a tyrkysový.
- Vrtulník a jeskynní muž (krok 16): textury z kroku 14 (kůra `palm_bark` na kmeny a pařezy, `rock_face_03` na
  kámen) a procedurální textury spočítané skriptem (bambus s kolénky, proutí s tmavými mezerami - od kroku 24a ne
  vyříznutými, díry jemnější než texel stínové mapy slunce dělaly na stěně kostičkovaný stín -, kůže, kost, provaz,
  list s žilkami; kůže člověka, vlasy, leopardí kožešina). Barvy, které hra mění (kůže vrtulníku, listy,
  vlasy, kožešina), jsou faktory základní barvy glTF nad světlou texturou.
- Voda (krok 19): moře kolem útesu, materiál Single Layer Water `M_UghWater` (`Shaders/UghWater.hlsl`): engine
  kreslí, co je pod hladinou, skrz tolik vody, kolik pohled projde (lom, pohlcování červené, rozptyl do modrozelena),
  a odrazy hladiny Lumenem; hladina je plochá přesně ve výšce `water_level` (mezi dvěma kroky interpolovaná), vlny
  jsou v normálách: příboj valící se k útesu a čeřiny po větru, kroužky kolem plavců a vrtulníku na vodě, kroužky
  kapek v dešti, pěna u skály (dýchající pruh, rozbitý šumem) a kolem plovoucích, kaustiky na tom, co leží pod
  hladinou (podle místa, kudy tam vniklo slunce); co je hlouběji, je tmavší a modřejší, zatopená jeskyně netemní jen
  rozptylem. Moře sahá od kamery (jeho řez je vidět, jen když stoupající voda přeroste kameru) přes zadní stěnu
  jeskyně a 200 m do stran - pro průlet nad mořem kroku 19e stačí zvětšit `UghWater::Reach`.

## Světlo a atmosféra

- Nálada levelu (krok 19, `UghMood`): klidné levely jdou dnem podle čísla v pořadí režimu (6 levelů: den, den,
  zlatý večer, večer, soumrak, noc), level s větrem je bouřka. Slunce vždy zepředu shora zleva (stíny skal padají na
  zadní stěnu, hloubka je čitelná), k večeru níž a tepleji (den 55° 10 lux, večer 32° 8 lux, soumrak 12° 3,5 lux
  oranžové), v noci chladný měsíc 0,6 lux a hlavní světlo dávají ohně; obloha HDRI nálady na kopuli kolem světa, kterou
  snímá sky light (osvětlení i odrazy ve vodě), Lumen GI s hardwarovým ray tracingem, pevná expozice pro každou náladu
  (den EV100 1,9, večer 1,4, soumrak 1,3, noc 0,6 - noc je tmavá, ale postavy čitelné; od kroku 24c den 2,1, večer
  1,6, soumrak 1,4). Objemová mlha sahá až k útesu (od 40 do 120 m od kamery), slunce v ní svítí do jeskyně (paprsky,
  slabé). Bez HDRI atmosféra enginu.
- Palmy byly šedé, protože materiály modelů (instance glTF materiálů enginu) nepovolují Nanite a hra kreslí místo
  nich výchozí materiál: import je teď přepojí na kopie v `Content/Imported/_Masters`, které Nanite povolují.
- Ohně (krok 18): až tři na nejdelších suchých římsách (napřed bez plošiny), za deskou hry: kamenný kruh a polena
  (Kenney, v tmavší plastelíně), plamen ze tří zkřížených kartiček (materiál `M_UghFire`, `UghFlame.hlsl`: jazyky ohně
  olizují vzhůru stoupajícím šumem, jiskry, každá kartička jinak) a blikající oranžové bodové světlo každého (Lumen),
  každý oheň jinak; zhasne, když voda vystoupí nad jeho římsu. Od kroku 19g jinak (viz Oheň a louče níže).
- Mlha: nízká přízemní (exponential height fog) s objemovou mlhou, ať je hloubka čitelná.
- Vítr (krok 12, 19): level s větrem je bouřka - slabší chladné světlo, zatažená obloha (`sky_kloofendal_overcast`
  ztmavená), hustá šedá mlha a mlžný opar nad vodou, rostliny vzorku Electric Dreams se klátí silněji a po větru
  (jejich `MPC_GlobalFoliageActor`), plameny ohňů se kloní po větru a víc blikají. Déšť (`AUghRain`) padá jako kapky
  logiky, stejně daleko po větru jako dolů: 6000 čar mezi kamerou a útesem (síť drobných čtverců, které posouvá
  materiál `M_UghRain`, `UghRain.hlsl` - GPU, nic za snímek), šplouchnutí na římsách a na vodě (`M_UghSplash`)
  a kroužky kapek na hladině; kapky logiky (originál je kreslí) jsou světlé čáry na kartičkách v rovině hry. Niagara
  ne: její systémy jsou binární assety z editoru, ne kód - stejný výsledek dává materiál.
- Menu: za ním ztlumené dioráma levelu, který by menu spustilo (po heslu ten level).

## Kamera

Pevná, úzký objektiv (30°), celý řez v záběru, mírně shora (-4°), aby byly vidět horní plochy plošin. Na začátku
levelu k ní kamera přiletí nad mořem (krok 19e, níže).

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
  Krok 19 (moře Single Layer Water, objemová mlha až k útesu, déšť): jednotlivé snímky 37-44 fps; v dlouhém
  `levels.ps1` notebook po pár minutách přejde do úsporného stavu GPU (snímky ~25 místo ~40) - medián 24, s hrubší
  mřížkou objemové mlhy a odrazy moře v polovičním rozlišení (`DefaultEngine.ini`) 26, nejpomalejší 21.
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
- **oblohy**: `sky_belfast_sunset` (zlatý večer), `sky_kloofendal_cloudy` (den s mraky), `sky_qwantani_dusk` (soumrak),
  `sky_qwantani_night` (noc s Mléčnou drahou), `sky_kloofendal_overcast` (zataženo - bouřka; krok 19),
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
nic není na obrazovce blíž než `SlabFront` (25 jednotek za rovinou hry: deska hry), nic vyššího než
pokryv země (5 px: tráva, květiny, kosti, malé kameny) blíž než `FigureReach` (80: tělo vrtulníku 45, triceratops,
foukač a strom s tváří 70), nic vyššího než 16 px ani liána blíž než `SweepReach` (180: rotor 130, křídla letce 170);
kde přistává vrtulník na plošině, tam kde je jeho tělo (11 px do stran, 20 nahoru) nic blíž než 80, kde se točí
rotor (13 px do stran, 16-24 px nad plošinou) nic blíž než 180. Listnaté rostliny se smí prolínat (od sebe mají
jen prostředky), tráva roste i u kmene palmy, do ohně nezasahuje nic; před cedulí plošiny nic (v jejím obdélníku
nic blíž než její zadek, 108 jednotek).

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
| útesy, skalní stěny (Nanite) | `Megascans/3D_Assets`: `HugeSandstoneCliff` (8 sítí), `MassiveSandstoneCliff` (6), `MossyRockFace` (3), `QuarryCliff`, `HugeMossyForestCliff`, `ForestRockFormation`, `CanyonSandstoneRidge`, `HugeNordicCoastalCliff`; `SmartAssets/AssetMeshes` `SM_Cliff_01..19`; `Meshes/_GENERATED/RockFormation` | 1,7 GB, 1,0, 0,9, 1,0, 0,09, 0,7, 0,6, 0,3; 4,1; 3,2 | `HugeSandstoneCliff_01`, `_03`, `MassiveSandstoneCliff_02` (zadní stěna); od kroku 19c šedé `HugeNordicCoastalCliff_01/_02`, `MassiveNordicCoastalCliff_01` |
| balvany, kameny | `SandstoneBoulder`, `MossyForestBoulder`, `MossyForestRock`, `LichenedForestBoulder`, `MossyRocks`, `NordicBoulder`, `SmallStonesPack`, `TundraMossyBoulder` | 0,6, 1,0, 0,4, 0,06, 0,7, 0,2, 0,3, 0,2 GB | `SmallStonesPack_01/03/05`, `MossyForestRock_01`, `LichenedForestBoulder_01` (kameny na římsách) |
| kořeny | `Custom/RootsTest` (`SM_Roots_01..14`, `SM_ErosionRoots_01..04`), `Megascans/3D_Assets/ForestRoots`, `MossyForestRoots` | 3,3 GB (z toho 2,2 erozní kořeny), 0,6, 0,5 | `SM_Roots_01..05` (visí ze stropů jeskyně) |
| pařezy, kmeny | `TreeStump`, `RottenTreeStump`, `BrokenStump`, `BirchTreeStump`, `FallenTreeAssembly`, `OldFallenTree`, `DeadTree` | 0,2, 0,3, 0,4, 0,3, 0,3, 0,3, 0,3 GB | `TreeStump_01`, `RottenTreeStump_02` |
| palmy | `Megascans/3D_Plants`: `AlexandraPalm`, `ArecaPalm`, `CatPalm`, `CanePalm`, `FanPalm`, `BroadleafLadyPalm`, `ZamiaPalm` | 23-48 MB každá | `AlexandraPalm_01..03`, `FanPalm_01`, `ArecaPalm_03`, `CatPalm_01` |
| rostliny s velkými listy | `Taro`, `BirdOfParadise`, `CastorOilPlant`, `ArrowheadPlant`, `PinkCordyline`, `VariegatedCroton`, `JungleGinger`, `MoneyPlant`, `DragonTree`, `JanetCraigDracaena` | 22-59 MB | `Taro_02/05/06/10`, `ArrowheadPlant_04`, `PinkCordyline_02`, `VariegatedCroton_01`, `BirdOfParadise_01` (rostliny), `ArrowheadPlant_01/02/05`, `VariegatedCroton_02`, `CastorOilPlant_01` (keře) |
| kapradiny | `Fern`, `BeechFern`, `SilverLadyFern`, `BostonFern` | 29, 195, 57, 12 MB | `Fern_01/02/05`, `BeechFern_04`, `SilverLadyFern_01`, `BostonFern_02` |
| tráva, květiny, pokryv | `KikuyuGrass`, `GroundCover`, `CloverVarieties`, `WhiteWindflower`, `RedLachenalia`, `Amaryllis`, `Periwinkle`, `LilyOfTheValley`, `CustomMoss`, `DeadLeaves` | 3-52 MB | `KikuyuGrass_01/03/05/06`; `WhiteWindflower_01/03`, `RedLachenalia_06`, `Amaryllis_03`, `GroundCover_07` |
| liány | `Megascans/3D_Plants/Ivy` (25 sítí), `EnglishIvy`, `Custom/IvyTest` (`SM_HangingVine_01..11`, `SM_CliffVine_01..09`) | 109, 16, 1 870 MB | `Ivy_11/13/15/17/21/23`, `HangingVine_09/10` (ze stropů), `Ivy_09/12/16/18/19/22` (závěsy po zadní stěně) |
| povrchy (dlaždicové) | `Megascans/Surfaces`: `BeachCliff`, `JungleGround`, `MossyGrass`, `NordicMoss`, `MossyRockyGround`, `ButtressRoot`, `IcelandicQuarryRock`; `SmartAssets/TileableTextures` `T_Rock_01..03`, `T_Moss_02`, `T_Lichen_01` | 0,9 GB; 0,2 GB | vrstvy skály: `BeachCliff` (pískovec, krok 18c), `T_Rock_03` (šedá), `MossyGrass`, `NordicMoss`, `JungleGround`; od kroku 19c jen poslední tři (skála je volný vápenec) |

Náhledy pro výběr byly miniatury uložené v hlavičkách `.uasset` (bez spuštění editoru vzorku). Odmítnuto: textury
naskenovaných skal (`BeachCliff`) se na velké skále zjevně opakují (krok 18c je bere znovu, ve dvou měřítkách
střídaných podle velkých skvrn); skenované útesy jsou zezadu otevřené (jen čelem ke kameře); balvany a kořeny vsazené do čela skály v rovině hry vypadaly jako nalepené oblázky (pevné oblasti masky jsou
na ně tenké) - čelo skály zůstává sítí z pole s naskenovaným materiálem.

### Jak se to používá

- **Zadní stěna jeskyně** (`UghRockDressing`, `AUghCliffDressing`): velké naskenované pískovcové útesy (poloměr
  40-65 px, mřížka 24 px i za okraji obrazovky, natočené ke kameře ±15°, překrývají se) vsazené do zadní stěny, ven
  45-70 % hloubky modelu; nikdy blíž než 250 jednotek (dosah rotoru a křídel 180) ani blíž než prostředek dekorace
  před nimi (kromě pokryvu a lián). Kořeny visí ze stropů (každý druhý sloupec po 5 px pod stropem s 14 px místa) před
  útesem, který tam je. Test `Ugh.Dressing` (150 levelů: aspoň 10 útesů, nejméně 23, deterministicky, za dosahem
  postav a za dekoracemi).
- **Čelo skály** (`M_UghCliff`, krok 18c): stejný triplanár, vrstvy z naskenovaných povrchů vzorku: pískovec
  `BeachCliff` (vrstevnatý, popraskaný; 5 m a 7 m na dlaždici s posunem, která z nich, rozhodují velké skvrny a jejich
  reliéf - dlaždice se neopakují), šedá `T_Rock_03` (skvrny v pískovci, zadní stěna, jemný reliéf normál na celé skále
  3,3x hustěji), `MossyGrass`, `NordicMoss`, `JungleGround`; reliéf podle kanálů `HeightMask` rozhoduje přechody.
  Tráva na plochách nahoru a pruh přes horní hrany masky (barva vrcholů modrá: 2-8 px pod hranou, kde je nad ní
  vzduch), pod ní mech; hlína na svazích a ve škvírách ploch nahoru; velké skvrny (alfa vrcholů, Perlin po několika
  metrech) světlejší / tmavší, teplejší / šedší; škvíry sítě a prohlubně reliéfu tmavší (i AO); u vody a pod ní mokrá
  skála tmavší a lesklejší (parametr `WaterLevel`, `AUghBackground::SetWater`); pískovec ztlumený do okrové jako útesy
  vzadu, kresba tónuje jen trochu. Geometrie: reliéf čela (vrstvy, pukliny, boule) jen před deskou hry, hrana v rovině
  hry je dál maska (test `Ugh.Rock`). Bez kopie vzorku stejný materiál s CC0 sadami kroku 14.
- **Dekorace**: druhy kroku 18 dostanou modely vzorku (`UghElectricDreams.h`: palmy s velkými listy, rostliny džungle,
  kapradiny, keře, tráva, květiny, kameny, pařezy, liány břečťanu); kosti, totemy, chýše a ohně zůstávají z Blenderu
  a Kenneyho. Pravidla chráněných objemů beze změny (`Ugh.Scenery`).
- **Světlo a post**: expozice EV100 1,5 (o půl clony světlejší), jemný bloom 0,35, teplé vyvážení bílé 6100 K,
  sytost a kontrast +8 % / +6 %, vinětace 0,3; Lumen a mlha jako dřív.
- Bez kopie vzorku hra ukáže CC0 assety kroku 14-18 (a jednou to zaloguje).

## Fotorealistické postavy z MetaHumanů (krok 18d)

Jan: postavy z Blenderu jsou špatné, chce fotorealistické lidi. Zdroj: **MetaHuman Creator** v editoru (plugin
`MetaHumanCharacter` UE 5.8 s volitelným obsahem „MetaHuman Creator Core Data“: 29 předloh postav, vlasy, vousy,
oblečení). Vzorek `..\Unreal Projects\MetaHumans` (UE 5.7) má jen dva hotové MetaHumany (Ada, Taro) bez stařce, proto
se nepoužil; předlohy Creatoru jdou upravit skriptem (Python API `MetaHumanCharacterEditorSubsystem`).

- **Kdo** (`Python/metahumans.py`, `UghMetaHumans.h`): pilot = mladý muž `Mateo` s dlouhými rozcuchanými vlasy
  (`Hair_L_MessyClumps`) a strništěm; cestující podle `cargo_look`: 1 muž `Bruce` (vlasy `Hair_M_Layered`, dlouhý
  rozcuchaný plnovous a knír), 2 žena `Celeste` (dlouhé rozcuchané zrzavé vlasy), 3 stařec `Walter` (dlouhé bílé
  vlasy, plný bílý plnovous); vzhled 4 zůstává kámen s očima. Barva vlasů jsou parametry materiálů vlasů
  (`hairMelanin`, `hairRedness`, `WhiteAmount`).
- **Oblečení** (do kroku 19b, teď listy - viz níže): jediný oděv Creatoru (tričko a šortky) dostane materiál leopardí
  kožešiny jeskynního muže z Blenderu
  (slot `fur` z `caveman.glb`; bez něj oděv v barvách kůže a kožešiny). Holé tělo pod tričkem MetaHuman nemá (tělo je
  pod oděvem vyříznuté), proto všichni nosí tuniku.
- **Sestavení**: obličejový rig a textury kůže stáhne Creator z Epicova cloudu (editor přihlášený k Epic účtu), pak
  sestavení `Optimized` v kvalitě `Low` (zapečené textury, vlasy jako vlákna pro detail 0-1 a „helmy“ - síťové čepice
  s texturou vlasů - pro 5-7; karty kvalita Low nevyrábí) do `Content/External/MetaHumans/<jméno>`, sdílené assety
  v `.../Common` (gitignore, asi 1,5 GB). Bez spuštěného editoru (commandlet) sestavení selže na grafu textur, proto
  `metahumans.ps1` pouští editor bez okna (`-RenderOffscreen`).
- **Ve hře** (`FUghMetaHuman`): komponenty blueprintu `BP_<jméno>` bez actora - tělo hraje akci, obličej a oděv jdou
  za jeho pózou (leader pose), vlasy, vousy, obočí jsou groomy na obličeji v detailu 5 (helma; vlákna jsou na malé
  postavy drahá) bez fyziky. Postava je zmenšená na výšku jeskynního muže (115 cm), takže místa a měřítka kroků 16-17
  (`UghFigurePlace`, `UghCopterModel`) platí beze změny.
- **Akce** (`Python/metahuman_actions.py`, `metahuman_poses.py`, `ugh_math.py`): pro každou postavu v jejích
  proporcích; `idle` a `walk` jsou klipy Creatoru (stání, chůze vpřed - na místě), ostatní pózy počítané skriptem
  (dvoukostrové IK, natočení páteře a hlavy, prsty v pěst): `wave` volání oběma rukama nad hlavou, `sit` ruce na
  kolenou, `pedal` nohy na pedálech kliky a ruce na řídítkách (čísla `Blender/copter_layout.py`), `hang`, `tread`,
  `swim`, `fall`. Počátky akcí jako u jeskynního muže. Test `Ugh.Figures.People`.
- Bez MetaHumanů (repo bez externího obsahu) zůstává jeskynní muž z Blenderu, bez něj plastelína.

## Lidé: listy, vlasy, sezení (krok 19b)

Jan: místo leopardích triček zakrýt intimní partie velkými listy, skutečné vlasy a vousy, pilot i cestující v kabině
oba sedí a jsou stejně velcí jako při chůzi, kámen vezený v kabině sedí zmenšený na sedadle.

- **Bez oděvu, celé tělo** (`Python/metahumans.py`): slot `Outfits` zůstává prázdný, takže sestavení nic z těla
  nevyřízne; tělo MetaHumana má pod tím spodní prádlo v texturách kůže (u ženy i horní díl), nic intimního.
- **Listy** (`FUghLeaves`, `UghLeaves.cpp`): bederní rouška z velkých listů tara vzorku Electric Dreams
  (`UghElectricDreams::LeafMaterial`, výřezy listů v jeho atlasu `LeafPictures`: od špičky po řez nad zářezem řapíku,
  takže list visí z rovné hrany); každý list je síť 4 x 6 čtverců ovinutá kolem těla kousek od něj, dolů se rozevírá
  a listy se po vrstvách překrývají. Vpředu a po stranách listy na stehnech (`thigh_l`/`thigh_r`: jdou s nohama, při
  sezení leží na klíně), vzadu a krátký list před rozkrokem na pánvi; žena má navíc pás listů přes prsa (`spine_05`).
  Kde tělo je, říká fyzikální asset postavy (kapsle pánve, páteře a stehen v klidové póze, řez po výškách), sítě se
  postaví za běhu pro každé tělo v prostoru své kosti a připojí se ke kosti (komponenty statických sítí, bez kolize);
  vítr materiálu vypnutý. Bez kopie vzorku zelená plastelína. Test `Ugh.Figures.People` (listy kryjí boky od pasu po
  stehna, u ženy i hruď).
- **Vlasy a vousy**: sestavení v kvalitě `Medium` - groomy mají karty vlasů jen v detailu 3 (vlákna 0-1 jsou na malé
  postavy drahá, helmy 5-7), hra drží detail 3 (`GroomLOD`), bez fyziky; obočí jsou teď také vidět (dřív helmy
  neviditelné).
- **Velikost** (`UghFigurePlace::PersonHeight` 145 cm vestoje, `copter_layout.PERSON_HEIGHT`): chodící postava s vlasy
  vyplní výšku spritu cestujícího (14 px; při chůzi je o ~5 % nižší), stejně velká stojí, čeká, plave i sedí ve
  vrtulníku (dřív 115 cm v kabině a natažená podle výšky spritu na zemi). Jeskynní muž z Blenderu se škáluje na stejnou
  výšku (jeho pedály a řídítka v `caveman_actions.py` přepočtené).
- **Sezení v kabině** (`copter_layout.py`, `UghCopterModel.h`): pilot sedí natočený o 50° (od kroku 24b 70°) doleva, cestující o 50°
  doprava, takže kamera zepředu vidí stehna a sezení z boku; klika s pedály a řídítka jsou v rámci pilota (natočené
  s ním, `PedalAxle`, `Grip`), pedály dál a níž pro delší nohy, sedadlo pilota o 8 cm dozadu, aby řídítka zůstala
  v ±45. Akce MetaHumanů jsou udělané pro jejich tělo (`retarget_source_asset`): engine dřív pánev „přetargetoval“ ze
  skeletu do proporcí těla a sedící a mávající postavy se vznášely ~7 cm nad sedadlem a zemí; test teď počítá
  pózy jako hra.
- **Kámen v kabině**: veze-li vrtulník kámen s očima (vzhled 4) a nevisí-li pod ním, sedí na sedadle cestujícího
  zmenšený na 0,32 a dívá se do kamery (`AUghCopters`, `SeatedStone`; `shot.ps1 -Cargo 4 -CloseUp`).
- **Výkon**: karty vlasů stojí na Radeonu 890M asi 1 ms snímku (interpolace karet, BLAS paprsků, base pass), proto
  lidé nejsou ve scéně ray tracingu (`SetVisibleInRayTracing(false)`: odrazy Lumenu by je stejně neukázaly) a jinde
  se šetří, co není vidět (od kroku 22 předvolba kvality Epic v `UghGraphics.cpp`, dřív `DefaultEngine.ini`): odrazy
  Lumenu v polovičním rozlišení, stínové mapy slunce o 2,5 úrovně hrubší, objem osvětlení průsvitných věcí 32 buněk.
  Snímek GPU (team-21) 23,1 -> 19,9 ms; `levels.ps1` medián 25 fps.

## Skály podle reference: šedý krasový vápenec, vchody do jeskyní (krok 19c)

Jan: pískovec působí „lakovaně“; skály mají vypadat jako na jeho fotce vchodu do jeskyně (šedý zvětralý vápenec,
ostré lomy a římsy, tmavé škvíry, mech a lišejník, matné, mokré jen u vody) a dveře kresby, odkud vycházejí cestující,
mají být skutečné vchody do jeskyně, ne díra do skály.

- **Čelo skály** (`M_UghCliff`, `UghCliff.hlsl`): vrstva skály je volná sada Poly Haven `marble_cliff_03` (rozpukaný
  útes z bloků a říms, 4,5 m na dlaždici, dvě měřítka proti opakování) převedená do světle šedého, trochu teplého
  vápence s vyšším kontrastem a tmavými stékanými pruhy zvětrání (její reliéf natažený dolů); šedý kámen
  `mossy_rock` (šedá skála s lišejníkem) ve skvrnách vápence a na stěnách jeskyně; mech ve škvírách, na malých římsách
  a v části velkých skvrn; tráva jen na skutečných plochách nahoru (normála nad 0,75) a přes horní hrany; hlína jen
  ve škvírách ploch nahoru; suchá skála vždy matná (drsnost aspoň 0,75), mokrá jen do metru nad vodou; kresba už
  netónuje barvu, jen světlost. Vrstvy trávy, mechu a hlíny dál z naskenovaných povrchů vzorku (pískovec `BeachCliff`
  a `T_Rock_03` vypadly z kopie), skála a šedý kámen z volných sad - každá vrstva zvlášť (`AUghBackground`).
- **Reliéf čela** (`FUghRockField::Front`, `UghRockNoise`): místo vrstev pískovce rozpukané bloky vápence - buňky
  rozházené mřížky (asi 16 x 9 px, na nich menší po 40 %), každý blok vystupuje po svém s rovnou, trochu nakloněnou
  plochou, škvíra jen na části hranic (jinak jen schod); pořád jen před deskou hry (`Ugh.Rock`).
- **Útesy vzadu** (`UghRockDressing`, `AUghCliffDressing`): pískovcové útesy nahradily šedé útesy vzorku
  `HugeNordicCoastalCliff_01/_02` a `MassiveNordicCoastalCliff_01`, položené na bok (jejich svislé sloupy leží jako
  lavice vápence, rovná pata se neukazuje jako police) a přebarvené novým materiálem `M_UghScan` (jejich vlastní
  textury: barva zbavená 70 % sytosti, světle šedý tón, tmavý mech na tom, co míří nahoru) - levnější než Megascans
  master vzorku s vrstvami mechu a oxidace. `MossyRockFace_03` vyzkoušen a zahozen (jako sýr s mechem).
- **Vchody do jeskyní** (`FUghCavePortal`, `UghCavePortals`): dveře kresby (2 x 2 dlaždice: dřevěný rám 60/61 nad
  80/81 nebo ústí jeskyně 62/63 nad 82/83, `FUghLevelArt::Doors`; 377 dveří v 81 levelech) jsou v poli skály oblouk
  rozpukaných bloků vystupující ze zadní stěny 18 px za rovinou hry (za dosahem rotoru a křídel), dozadu se rozšiřuje
  a splývá se stěnou, kolem otvoru 22 x 22 px; chodba za ním vede 72 px hluboko a stáčí se ke kraji obrazovky, takže
  její konec není vidět; podlaha je římsa masky. Stěny chodby jsou ve vrcholových barvách „zavřené“ (od 1 do 16 px za
  obloukem stále víc) - tmavé i bez světla. Cestující vychází z tmy 21 px hluboko (`UghFigurePlace::DoorDepth`).
  V otvoru se nic jiného neukáže: dekorace jen nízký pokryv na podlaze před obloukem, útesy a kořeny se zmenší nebo
  vypadnou, krápníky a spadané kameny před vchodem nejsou. Test `Ugh.Portals` (u každých dveří vchod na podlaze
  masky, otevřený od oblouku po místo, odkud vychází cestující, skála nad otvorem a vedle něj, podlaha, konec chodby).

## Cedule s čísly a bubliny (krok 19d)

- **Cedule** (`UghPadSigns`, `AUghSigns`, `Blender/signs.py`): zvětralé prkno (Poly Haven `rough_wood` do hnědé jako
  prkno originálu, zubaté okraje, mírně prohnuté) přibité dvěma kolíky na křivý kůl, číslo plošiny vyřezané čárkami jako
  sprity originálu (85-89: I až IIII, pět = čtyři přeškrtnuté pátou; 90 prázdné prkno): zářezy ukazují světlé čerstvé
  dřevo, takže jsou čitelné i z dálky a v noci. Velké jako dlaždice originálu (prkno 1,46 x 0,8 m, 0,3 m nad zemí, kůl
  1,22 m). Každá plošina má právě jednu: kde ji kresba originálu na plošině má (spodek dlaždice na povrchu plošiny),
  stojí tam, jinak co nejblíž středu plošiny mimo vchody do jeskyní a jiné cedule (47 plošin, kam žádný cestující
  nejezdí, originál nechal bez cedule). Značky jsou číslo plošiny z logiky (`ugh_logic_get_pad`), ne sprite: kresba
  se s ním shoduje až na jedno přehlédnutí (level_id 31 má II i na plošině 1) a jednu druhou ceduli na téže plošině
  (level_id 74, II dvakrát: stojí první). Plošina čísla 6 (originál ji nepojmenoval) dostane prázdné prkno. Cedule
  stojí na své zemi 88 jednotek za rovinou hry (přední kolíky 81: za dosahem těl vrtulníků a nepřátel, test
  `Ugh.Signs`), dekorace se jim vyhnou (`FUghPlacer`). Bez modelů karta se spritem originálu.
- **Bubliny** (`UghBubbles`): obrázek kreslený za běhu ze vzdáleností (8 texelů na pixel: ostrý), bílá bublina
  s břidlicovým obrysem, uvnitř prkénko plošiny, kam chce cestující (stejné čárky jako cedule; od šesté plošiny prázdné
  jako v originálu), nebo otazník (vrtulník odletěl bez něj). Co ukazuje, říká logika jménem spritu
  (`destinationBubble` s indexem plošiny, `impatientBubble`). **Zobáček:** originál má jen sprity se zobáčkem vlevo
  dole a bublinu kreslí vždy 11 px vpravo a 13 px nad cestujícím (`Frame.kt`), zobáček tedy míří dolů na hlavu
  cestujícího (ne k vrtulníku). Dva zobáčky byly chyba kartičky: krychle s oboustranným materiálem ukazovala zezadu
  zrcadlený obrázek tam, kde je vpředu průhledno - `M_UghSprite` je teď jednostranný. Bublina, která by vpravo vyjela
  z obrazovky, se zrcadlí na levou stranu cestujícího se zobáčkem vpravo (test `Ugh.Bubbles`).

## Úvod levelu: let nad mořem ke kameni (krok 19e)

- **Kámen** (`FUghStackField`, `AUghSeaStack`): level je vytesaný do čela velkého samostatného kamene v moři (krasová
  věž jako v zátoce Ha Long): asi 108 m široký, 52 m hluboký, 90 m nad hladinou, nahoře klenutý. Pole na hrubé mřížce
  (6 px = 0,6 m) a stejné surface nets jako skála levelu (`UghSurfaceNets` je teď šablona pro obě mřížky); boky
  a záda zaoblené, reliéf z velkých boulí, svislých žlábků od deště, vrstev vápence (u každé spodní hrany římsa,
  pod ní mírný převis), rozpukaných bloků a zářezu vln u hladiny; pata se pod vodou rozšiřuje. Čelo je kolem skály
  levelu rovné: rám o 1 m před rovinou hry (čelo skály je nejvýš 0,6 m), dál od otvoru vystupuje až o 2,2 m, takže
  level sedí ve vytesaném výklenku. Skála levelu sedí v dutině kamene asi 4 px uvnitř své mřížky (otevřený okraj
  mřížky je v kameni) a kámen drží i neviditelný „rubáš“ stínů `AUghBackground` (test `Ugh.Stack`). Materiál je
  `M_UghCliff` čela (stejný šedý vápenec, mech, tráva nahoře a přes hrany), vrstvy skály 2,5krát větší (z dálky
  se neopakují), kresba jen jako neutrální teplá skála. Džungle (`AUghScenery`, modely Electric Dreams): palmy
  12-22 m, keře, rostliny s velkými listy, kapradiny a kameny na vrcholu, keře a kapradiny na římsách, břečťan visící
  z horních hran stěn (asi 4000 kusů). Kámen je stejný pro všechny levely, vyrobí se při prvním letu (asi 1 s)
  a mimo let je schovaný: hře nic nestojí.
- **Let** (`FUghIntro`): při prvním popisku levelu (ne po havárii) kamera jako FPV dron: 8 snímků drží v černé (render
  se usadí), 0,3 s se rozsvítí, pak 4,5 s z 260 m před kamerou hry nízko nad vlnami (2,6 m nad hladinou) po křivce
  Catmull-Rom s kličkou vpravo a zpět, polovinu času plnou rychlostí (asi 90 m/s), pak brzdí do zastavení; náklon
  do zatáček (nejvýš 10°) z bočního zrychlení, lehké houpání výšky, objektiv z 72° na hráčových 30°, pohled
  z poloviny po dráze, z poloviny na level, nakonec jen na level; menší rozmazání pohybem; venku o 0,8 EV tmavší
  expozice, která se ke konci vrací na náladu levelu (jako oko vlétající do jeskyně). Poslední snímek je přesně
  kamera hry (`AUghStage::Fit`, test `Ugh.Intro`), do kamene nevlétá (aspoň 2 m od něj, nikdy blíž než kamera hry).
  Klávesa zbytek letu zrychlí na 0,6 s (vždy dřív, než začne hra: zhasnutí popisku a černá před hrou trvají 73
  snímků logiky); logika a její časování beze změny, klávesa jde do logiky jako dřív. Během letu, popisku, černé
  před hrou a jejího roztmívání je scéna vidět (dřív černá); po havárii zůstává černý popisek jako dřív.
  `-UghNoIntro` nebo `FUghIntro::bFlies` let vypne.
- **Moře** (`UghWater::Box(…, bOpenSea)`): během letu otevřené moře 4 km kolem (jinak stará krabice), daleko od kamene
  (`UghWater.hlsl`: od 25 m) vyšší vlny, bílé hřebínky, drsnější hladina, malé vlny s dálkou mizí (žádné třpytivé
  zrnění) a při pohledu skoro vodorovně se uklidní (jinak by zrcadlily tmu pod obzorem), voda bez dna rozptyluje
  světlo jako pár metrů hluboká (dřív černá). U útesu (co vidí kamera hry) beze změny.
- **Obloha** (`UghSky.hlsl`, `FUghMood::SkySeen`): kopule je 10 km daleko a mlha sahá jen 5 km, takže obloha nálady je
  vidět nad oparem moře; kamera ji vidí tak jasně jako světlo, které dává (sky light ji zachytává beze změny), v noci
  tmavší (noční HDRI je jasné jako denní). Ve hře obloha vidět není (do kroku 19f ji materiál četl špatně, níže).
- **Snímky**: `shot.ps1 -Intro <s>` (`-UghShotIntro`) uloží let v čase (začátek 0,4, střed 2,4, konec 4,5 = kamera
  hry); autopilot `shot.ps1` / `levels.ps1` let zrychlí první klávesou popisku, snímky hry beze změny.

## Pramen, potok, můstek a vodopád (krok 19f)

- **Kde** (`UghStreams::Plan`, deterministicky z masky a pole skály): na římse (řádek masky s aspoň 14 px vzduchu
  nad sebou), pod kterou je skála celou cestu až 3 px pod hladinu na začátku levelu (aspoň 12 px vysoko; sloupce
  potoka a 4 px kolem: vodopád padá jen před skálou, nikdy před vzduchem, kudy létají postavy, ani před vodou, kde
  plavou), dál než 16 px od konce plošiny a mimo místo přistání vrtulníku, cedule a vchody do jeskyní, se zadní stěnou
  aspoň 1,6 m za rovinou hry a podlahou celou cestu k ní; z kandidátů nejvyšší vodopád, nejblíž středu obrazovky,
  jeden na level (38 ze 150 levelů). Jen vizuální: logika o ničem neví.
- **Pramen**: díra v zadní stěně jeskyně, kde podlaha stoupá do stěny, vytesaná do pole skály (tunel 2,4 px vysoký,
  60 jednotek do stěny); u ní dva kameny a za nimi kapradiny (`UghPlans::AddSprings`).
- **Potok** (7 px široký): koryto 1,5 px hluboké vytesané do skály před deskou hry a za ní (`FUghRockField::
  CarveChannels`, `UghStreams::Channel`; v desce -20 .. 20 jednotek zůstává maska, test `Ugh.Rock` i s korytem), voda
  0,9 px pod povrchem římsy proudí z díry ke kameře (kde podlaha stoupá do stěny, je to bílá kaskáda), deskou hry
  prochází skrytá ve skále a ústí zářezem v hraně čela.
- **Můstek** (`UghFalls::Bridge`): klády napříč korytem od hrany čela po 60 jednotek za rovinu hry na dvou trámech,
  jejich vršky těsně pod povrchem římsy (postavy po nich chodí, nic jim nezakrývá nohy), vzadu zábradlí (dva sloupky
  a tyč, 3,6 px: jako nízký porost, za postavami); textura `rough_wood`.
- **Vodopád**: ze zářezu přes hranu čela dolů do moře, před čelem aspoň o 6 jednotek, s pádem se vysouvá ke kameře
  (odmocnina výšky), trochu se rozšiřuje; síť materiálu `M_UghFlow` (`UghFlow.hlsl`: průsvitná osvětlená voda, vzor
  plyne s vodou podle času toku - pomalé čeření a pěna u břehů v korytě, bílé pruhy, chuchvalce a mezery mezi
  prameny ve vodopádu, dole bělejší; pod hladinou moře mizí, takže stoupající voda vodopád zkracuje a nakonec
  pohltí). Kde dopadá: moře pění a vře a běží z něj vlny (`UghWater.hlsl`, `Fall0..1`), stoupá mlha a tříšť
  (`M_UghMist`, `UghMist.hlsl`: obláčky na kartičkách, které posouvá materiál jako déšť kroku 19, nic za snímek,
  rostou, stoupají a mizí; Niagara ne - binární assety). Vše zůstává v ploše potoka (`FUghStream::Area`).
- **Pravidla** (testy `Ugh.Streams`, `Ugh.Scenery`, `Ugh.Dressing`, `Ugh.Rock`): pod potokem a vodopádem skála,
  daleko od plošin, cedulí a vchodů, klády mostu pod povrchem římsy a nad skálou, zábradlí za deskou hry a ne vyšší
  než nízký porost, vodopád před čelem a stále blíž ke kameře, pramen na zadní stěně, voda nad korytem; dekorace
  nestojí v korytě ani před pramenem (`UghStreams::Rooms`), útesy a kořeny zadní stěny nezakrývají pramen.

## Obloha a moře během letu (krok 19f)

- Obloha byla během letu jednolitě šedá: import udělá z long-lat HDRI krychlovou texturu (`TextureCube`), ale
  materiál oblohy ji četl jako 2D obrázek (engine pak dosadil výchozí šedou texturu - a tu snímal i sky light).
  `M_UghSky` teď čte krychli (`UghSky.hlsl`): je vidět obloha nálady s mraky a obzorem a sky light snímá skutečnou
  oblohu (osvětlení scény a odrazy ve vodě jsou teď podle nálady, ne šedé).
- Otevřené moře bylo skoro černé a zrnité (odrazy Lumenu z kamery letící nad vlnami a pod obzorem tma). Daleko od
  kamene (`UghWater.hlsl`) teď moře zrcadlí oblohu samo (`Mirror`: krychle oblohy nálady podle odrazu vlny a Fresnela,
  pod ní vlastní hluboká modř osvětlená oblohou), odrazy enginu tam slábnou; přibyly dlouhé vlny (34 a 21 m), aby byly
  hřebeny čitelné i zdálky. U útesu (co vidí kamera hry) beze změny.

## Oheň a louče (krok 19g)

- Plamen je zapečená simulace ohně (Jan chtěl Niagara Fluids): šablony 3D ohně pluginu (`Grid3D_Gas_Fire` …) kreslí
  heterogenními objemy, které v tomto projektu nekreslí nic (ani v hlavním pohledu, offscreen i se scene capture -
  vyzkoušeno, bake hrou bez okna fungoval jen s 2D šablonou `Grid2D_Gas_SmokeFire`, což je pevná scéna ohně
  rozfoukaného do strany bez parametrů). Proto vlastní offline simulátor plynu (`FUghFireSim`, commandlet
  `UghMakeFlames` v `build.ps1`, ~2 min): stabilní tekutina na posunuté mřížce 160 x 320 (Stam), palivo přiváděné
  šumem na loži hoří, kde je horko, v teplo a trochu sazí; teplo stoupá (vztlak), víry drží vorticity confinement
  a rozvíří je stoupající šum dvou velikostí, teplo a palivo nese MacCormack (ostré jazyky), tok bez divergence
  (tlak Gauss-Seidel s převolněním), teplo chladne. Světlo plamene = barva žhavého plynu (tmavě červená, oranžová,
  žlutá, světle žlutá) krát teplo na 4; plamen = tři vrstvy (tři simulace s jiným seedem kousek vedle sebe, jako hloubka
  skutečného plamene). `UghFlipbook`: bílá = 98,5. percentil jasu, ořez na místo, kde hoří v průměru všech snímků,
  posledních 16 snímků přechází do prvních (smyčka bez skoku), 8 x 8 snímků 128 x 256 při 30 fps (2,1 s), textury
  `T_UghFlameCampfire` / `T_UghFlameTorch` v `Content/Generated` (bez streamování - karta ukazuje malý kus textury
  zblízka), náhled `Saved/Flames/*.png`.
- Materiál `M_UghFlame` (`UghFlame.hlsl`) hraje flipbook na dvou zkřížených kartičkách (čelem ke kameře a napříč;
  kartička viděná z boku mizí), dva snímky prolnuté, každá kartička od svého snímku, ve větru se plamen naklání,
  lehce se tetelí. Jiskry (`M_UghSparks`, `UghSparks.hlsl`) a tenký dým (`M_UghSmoke`, `UghSmoke.hlsl`) jsou čtverečky,
  které posouvá GPU (jako déšť a mlha u vodopádu): jiskry vyletí, zpomalí, víří, chladnou ze žluté do červené; dým
  stoupá, roste a řídne, unáší ho vítr.
- Ohniště (`FUghHearths`): kruh naskenovaných kamenů `SmallStonesPack` vzorku Electric Dreams, uprostřed větve
  `OldTreeBranch` / `DryBranches` opřené o sebe jako stan, spálené na uhel, a řeřavé uhlíky (malé kameny) -
  materiál `M_UghEmbers` (`UghEmbers.hlsl`): černé popraskané dřevěné uhlí, trochu popela nahoře, v prasklinách
  žhnoucí žár, který pomalu dýchá, u polen víc zespodu. Bez vzorku Kenneyho ohniště v plastelíně.
- Louče (`AUghTorches`, `Blender/torch.py`): křivá násada, hlavice z lýka a smůly svázaná provázky (slot `char` žhne),
  zaražená do skály, nakloněná ven a trochu stranou, malý plamen (`T_UghFlameTorch`), pár jisker, chomáč dýmu.
  Rozmístění (`UghTorchPlan.cpp`, deterministicky): vedle vchodů do jeskyní na straně, kam se chodba nestáčí (při
  málo vchodech i na druhé), 7 px nad podlahou, pak na zadní stěně nad nejdelšími římsami daleko od ohňů; nejvýš 4
  na level, vždy za dosahem rotorů a křídel (`SweepReach`, jako liány), zaklíněná do stěny (`FUghPlacer::Settle`).
- Světlo (`UghFireParts`): bodové světlo každého ohně a louče bliká šumem tří rychlostí - jas, teplota barvy (1850 K,
  tmavší = červenější) a poloha (pár cm: stíny a odlesky na stěnách tančí), měkké stíny (poloměr zdroje), Lumen barví
  skálu teple; jas podle nálady (`FUghMood::FireLight`: den 1, večer 1,05, soumrak 1,15, noc 1,3, bouřka 1,1), oheň
  9 cd, louč 0,8 cd. Pod vodou ohně i louče zhasnou.

## Efekty událostí (krok 20)

- Každá událost logiky má efekt (`FUghEffectPlayer::Cues`, tabulka v `5_remake/game/README.md`, „The events seen“)
  a zvuk i efekt jdou ze stejné události: `UghEvents::Play` dá každou událost v jedné smyčce přehrávači zvuků
  i efektům. Co událost nemá, se čte z pohledu: přistání vrtulníku na plošině (prach podle rychlosti klesání,
  usednutí při vznášení ne), dopad bonusu na zem (trocha prachu), havárie do vody (k výbuchu šplouchnutí).
- Niagara ne (binární assety z editoru): efekty jsou jako déšť kroku 19 sítě drobných čtverečků, které hýbe
  a kreslí materiál na GPU (`UghBurst.hlsl`, `UghBurstLook.hlsl`): částice vyletí z krabice do kužele, brzdí ji
  vzduch, padá (nebo stoupá), kolébá se, převrací se a leží na zemi pod sebou; pod hladinou zmizí. Tři materiály:
  `M_UghBurst` (průsvitný, osvětlený: prach, kouř, tříšť, kroužky na vodě), `M_UghBits` (vyříznutý, osvětlený,
  převrací se i se světlem: třísky, listí, peří, mušle, okvětní lístky), `M_UghGlint` (světlo: oheň výbuchu, jiskry,
  třpyt, kapky). Za snímek se nastaví jen čas; od každého efektu nejvýš 3 naráz (nejstarší ustoupí), dvě světla
  záblesků bez stínů, body za zaplacení a omráčení stoupají jako text (HUD).
- Efekty (`UghBursts.cpp`): šplouchnutí, příboj u kamene při popisku (vidět z letu), výbuch (ohnivé jazyky,
  kouř, třísky, jiskry, záblesk), prach, mušlové peníze, třpyt (u bonusu barva podle druhu), peří, proud vzduchu pod
  křídly letce (smyčka do konce mávání), poryv foukače (listí, šmouhy větru, prach), dopad omráčeného (prach,
  kamínky), listí ze stromu, oslava (okvětní lístky, třpyt, záblesk). Krátké (nejvýš 6 s), prach a kouř kryjí
  nejvýš 0,7.
- `shot.ps1 -Effect all` (`-UghShotEffect`): každý efekt zastavený ve svém okamžiku u prvního vrtulníku (vedle něj
  ve vzduchu, na zemi nebo na vodě pod ním), detailní snímek `1p-01-<efekt>.png`.

## Menu a HUD (krok 21)

- Obrazovka ve Slate stavěná v kódu (`UghUi*`, žádné binární widget blueprinty), škálovaná DPI křivkou enginu
  (navrženo pro 1080 řádků, čitelné i v 720). Barvy: teplá kost a jantar ohně na tmavém skle (průsvitné panely se
  zaoblenými rohy a tenkým teplým okrajem). Písma zdarma (OFL, Google Fonts přes `Assets.json` do
  `assets/3d/googlefonts`): Lilita One na titulky a čísla, Alegreya Sans na text; bez nich Roboto enginu.
- Obrázky kreslí kód při startu (`UghStoneArt`): logo „UGH!“ z tlustých kamenných písmen (vzdálenostní pole tahů,
  každé písmeno trochu nakloněné), kamenná deska popisku, kostěný vrtulník života a kost ukazatele energie - zkosená
  hrana, zrno, křivé praskliny, mech nahoře, tmavý lem, měkký stín.
- Titulní obrazovka: za ní kamera pomalu krouží kolem kamene v moři (`UghMenuView`, vytesaný level, který by menu
  spustilo), vlevo ztmavení, logo, řádky menu (hráči, obtížnost, pole hesla s kurzorem a levelem, Hrát, Konec; vybraný
  jantarově), poslední hra, klávesy v jednom řádku. Konec hry: kamenná deska „GAME OVER“ / „ALL LEVELS DONE!“, panel
  s levelem, skóre a režimem, „Press any key“.
- Hra: dva panely nahoře (level, životy jako kostěné vrtulníky, energie jako kost se žlábkem zelená / jantarová /
  červená pulzující; skóre a násobitel), mizí s prolínáním hry; žádný dlouhý řádek kláves - pomoc F1 (sama v levelu 1
  přes popisek a 6 s hry); popisek levelu jako kamenná deska s vytesaným číslem a heslem nad letem; body stoupají
  jantarově s obrysem; hlasitost a upscaler jako krátké oznámení.

## Nastavení a ovládání (krok 22)

- Obrazovky menu ve stejném stylu jako titulní (`UghUi*`): Settings a Controls stojí ve sloupci titulní obrazovky
  (velký titulek Lilita One s jantarovou linkou, skleněný panel, klávesy dole), scéna kolem kamene zůstává vidět
  vpravo; High scores jsou dvě tabulky vedle sebe uprostřed nad ztmavenou scénou. Vybraný řádek jantarové sklo se
  šipkami, hlasitosti jako řada kamínků, co řádek dělá, v panelu pod ním; klávesy pilotů jako tabulka políček (šipky
  větší, písmo záložní); jméno nejlepšího skóre jako řada políček na kartě konce hry (kurzor jantarový).
- Předvolby kvality (`UghGraphics.cpp`): skupiny škálovatelnosti enginu na úrovni předvolby (nízká se stíny slunce
  střední: dioráma bez nich nežije; od kroku 23 bez GI Lumenu, jen sky light) a těžké věci diorámatu:

| | Nízká | Střední | Vysoká | Epická (jak bylo vyladěno) |
|---|---|---|---|---|
| objemová mlha (paprsky slunce do jeskyně) | ne | buňky 24 px, 48 vrstev | 16 px, 64 | 16 px, 64 |
| odrazy moře | obloha a záchyty | Lumen, poloviční | Lumen, poloviční | Lumen, poloviční |
| lom moře | poloviční | poloviční | plný | plný |
| stínové mapy slunce (LOD bias) | 2 | 1,5 | 1 | 1 |
| objem osvětlení průsvitných | 16 | 24 | 32 | 32 |
| vlasy lidí (LOD groomu) | helmy (4) | helmy (4) | karty (3) | karty (3) |
| rozlišení pro upscaler (krok 23) | 50 % | 58 % | 67 % | 67 % |
| stíny ohňů a loučí (krok 23) | ne | ano | ano | ano |
| světla ohňů se kmitají s plamenem (krok 23) | ne | ne | ne | ano |

Krok 23: na Radeonu 890M byly stíny ohňů (bodová světla: krychle šesti stínových map) hlavní pevná cena snímku -
bez nich je nízká dvakrát rychlejší; světlo, které se hýbe, kreslí své stínové mapy každý snímek znovu (virtuální
stínové mapy nehybného světla drží stránky), proto se pod Epic nehýbe. Hra bez profilu začne na předvolbě podle GPU:
RTX (DLSS) Epic, integrovaná Low, jinak High.

## Čitelnost: postavy nad skálou (krok 24c)

Jan po prvním hraní: scéna přesvětlená, postavy (čekající cestující, piloti, nepřátelé) splývají se skálou. Hierarchie
v rovině hry: postavy > plošiny > skála, v každé náladě a předvolbě.

- **Skála a expozice**: vápenec čela skály tmavší (`UghCliff.hlsl`: šedá 0,16 místo 0,22, tedy střední, ne světle
  šedá), expozice světlejších nálad o kus níž (EV100 den 2,1, večer 1,6, soumrak 1,4, bouřka 1,15; noc 0,6 beze změny).
- **Světla jen na postavách** (`UghFigureLook`, `AUghStage`): postavy (lidé, nepřátelé, bonusy, kámen s očima,
  piloti a cestující ve vrtulníku) jsou navíc ve světelném kanálu 1; dvě směrová světla jen v něm - výplň zepředu
  zprava shora a kontra zezadu shora (2x výplně, rozsvítí hlavy, ramena a ruce proti skále), barva slunce napůl
  k bílé, bez stínů, Lumen je neodráží (nepřidají světlo skále), nesvítí do mlhy ani deště. Jas
  `FUghMood::FigureFill` krát světlo, které expozice dělá střední šedí (2^EV), takže postavy svítí stejně ve dne i
  v noci (noc 0,9, jinak 1,2, bouřka 1,3). Funguje i na Low (přímé světlo, ne GI).
- **Tmavá svatozář** (`M_UghFigureHalo`, `UghFigureHalo.hlsl`): postavy se kreslí do custom depth, post-process
  kolem nich (2,4 % výšky obrazu, tři prstence po 8 vzorcích) jemně ztmaví pozadí, nejvíc u postavy (`FUghMood::Halo`
  0,5-0,6 = nejvýš o polovinu), ne kde je pozadí tmavé už samo (jeskyně). Až za upscalerem (`BL_SceneColorBeforeBloom`):
  před ním ležela svatozář pohybující se postavy na nehybném pozadí, jehož historii TSR/FSR drží - rozmazala se do
  ztracena. Piloti a cestující ve vrtulníku svatozář nemají (vrtulník je vidět sám, šmouha by ho zašpinila).
