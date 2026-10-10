# UGH! remake doma (RTX 5060 Ti)

Krátký návod, jak hru doma sestavit, zabalit a spustit, a co čeká na tvoje posouzení. Všechny příkazy jsou pro
Windows PowerShell 5.1. Balíček i assety jsou jen pro vlastní použití - data hry, vzorek Electric Dreams, MetaHumani
a modely z Fabu/Sketchfabu nejsou naše a nikam se nenahrávají.

## Nejrychlejší cesta: hotový zip z notebooku

1. Na notebooku je po kroku 32 `5_remake\game\Packaged\UghGame-Windows.zip` (1,1 GB, rozbalený 1,3 GB; kroky
   24f-32g, zabaleno 2026-10-10). Přenes ho domů na disk (USB, síť), rozbal třeba do `D:\Hry\UGH`.
2. Spusť `UghGame.exe`. Je to Shipping build (bez logu a konzole, menší). Při prvním startu bez profilu hra sama
   vybere kvalitu podle grafiky: na RTX (DLSS) **Epic + DLSS**, na integrované grafice Low, jinak High. Volba se
   uloží do `%LOCALAPPDATA%\UghGame\Saved\UghProfile.json`, jakmile cokoli změníš v Settings. Replaye jsou vedle
   v `%LOCALAPPDATA%\UghGame\Saved\Replays` (zip žádné nemá, složka vznikne s prvním uloženým replayem).
3. **Frame generation**: Settings > Frame generation (2x / 3x / 4x - RTX 50 umí až 4x), nebo ve hře klávesa **G**.
   Upscaler přepíná **U** (DLSS / FSR / TSR). Zkušební průchod 2026-10-02 dal na RTX 5060 Ti ~150 fps bez a ~450 fps
   s frame generation.
4. Hra v okně nebo na celé obrazovce a rozlišení: Settings > Resolution, Window.

PSO cache je v balíčku (nahraná na Radeonu, ale je to popis pipeline, ne binárka ovladače - NVIDIA si ji při prvním
startu předkompiluje), takže první průlet nemá trhat.

## Hraní na notebooku (Radeon 890M)

```
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\play.ps1
```

- `play.ps1` teď spouští **zabalenou hru** (`Packaged\Windows\UghGame.exe`: uvařený obsah a PSO cache, hladká od
  prvního snímku); řekne, když je balíček starší než poslední build (pak `pso.ps1`). Profil (nastavení, klávesy,
  skóre) je pořád `5_remake\game\Saved\UghProfile.json`, replaye `Saved\Replays`, log `Saved\Logs\UghPlay.log`.
- `play.ps1 -Editor` je dřívější hra v editoru: necookovaný obsah, shadery se kompilují za běhu - hryže a je
  pomalejší, jen na rychlé vyzkoušení změny bez balení. Proto se dřív hra „nehratelně hryzala“.
- Alt+Enter (celá obrazovka) dřív přepnul grafiku potichu na Epic (engine znovu použil své uživatelské nastavení);
  opraveno v kroku 24f.
- Low drží 60 fps dynamickým rozlišením (33-50 % obrazovky, zahřátý notebook kreslí hrubší, ne pomaleji), Medium
  45 fps (42-59 %). High a Epic mají pevné rozlišení a vzhled beze změny.

## Sestavení doma od nuly

Jednou:

```
.\gradlew.bat :extractor:run
.\gradlew.bat :extractor:sound
powershell -ExecutionPolicy Bypass -File C:\Users\<ty>\IdeaProjects\UGH\5_remake\game\setup.ps1
powershell -ExecutionPolicy Bypass -File C:\Users\<ty>\IdeaProjects\UGH\5_remake\game\fetch-assets.ps1
powershell -ExecutionPolicy Bypass -File C:\Users\<ty>\IdeaProjects\UGH\5_remake\game\build.ps1
powershell -ExecutionPolicy Bypass -File C:\Users\<ty>\IdeaProjects\UGH\5_remake\game\electric-dreams.ps1
powershell -ExecutionPolicy Bypass -File C:\Users\<ty>\IdeaProjects\UGH\5_remake\game\metahumans.ps1
```

- `setup.ps1` potřebuje pluginy DLSS/Streamline/FSR z UghTrial (`-From <složka>`).
- `electric-dreams.ps1` potřebuje vzorek „Electric Dreams Environment“ (UE 5.8) v `..\Unreal Projects\ElectricDreamsEnv`
  vedle repozitáře (`-Source` jinde); bez něj hra ukáže CC0 assety.
- `metahumans.ps1` potřebuje volitelný obsah „MetaHuman Creator Core Data“ a editor přihlášený k Epic účtu; bez nich
  jeskynní muž z Blenderu.
- Janův T-rex a triceratops (`assets\3d\fab`, `assets\3d\sketchfab`) se kopírují ručně, pak znovu `fetch-assets.ps1`
  a `build.ps1`.

Testy: `test.ps1` (UE), `5_remake\logic\build.ps1`, `6_verification\build.ps1`.

## Balení

```
powershell -ExecutionPolicy Bypass -File C:\Users\<ty>\IdeaProjects\UGH\5_remake\game\pso.ps1
```

`pso.ps1` zabalí hru, nechá ji bez okna odehrát menu, let ke kameni, rychlou sadu levelů, efekty, výběr levelu,
rychlý let a předvolby Low a Medium s `-logPSO`, z nahrávky udělá PSO cache a zabalí znovu (asi 1 h). Jen balíček
bez nové cache: `package.ps1` (`-NoZip` bez zipu). Výsledek: `5_remake\game\Packaged\Windows\UghGame.exe`
(Development: log, konzole - pro `play.ps1`, `levels.ps1 -Package`, `perf.ps1`) a hra na hraní jinde
`Packaged\Shipping\Windows\UghGame.exe` (Shipping, stejný cook) se zipem `Packaged\UghGame-Windows.zip` (bez `.pdb`).

Velikost (krok 28): do balíčku jde jen to, co hra opravdu načítá (`UghCookList`), ne celé složky Electric Dreams,
nepoužité importy ani obličejové trackery MetaHuman Animatoru a denoiser path traceru (650 MB, pluginy je vařily
celé); textury světa a postav nejvýš 1024 (`Config\DefaultDeviceProfiles.ini`, kamera hry je daleko - beze změny
na snímcích), oblohy a plameny beze změny; Oodle Kraken úroveň 7; Shipping bez debug DLL DLSS/Streamline.

## Měření

- `perf.ps1` - fps zabalené hry bez okna pro Low/Medium/High/Epic (rychlá sada levelů, 1920x1080), tabulka
  `Saved\Shots\perf.txt`: medián, nejpomalejší a nejrychlejší level, 1 % low (fps nejpomalejšího procenta snímků)
  a hitche nad 50 ms. Bez okna nejede frame generation ani DLSS-G, je to čisté fps renderu. Porovnávat jen stejně
  zahřátý notebook (zahřátý je asi o třetinu pomalejší).
- `levels.ps1 -Package` - všech 150 levelů ze zabalené hry s archy snímků (asi půl hodiny).

## Co vyzkoušet nového (kroky 26-30)

- **Výběr levelu nad souostrovím** (26): PLAY proletí nad kameny, každý je level (zelený hotový, žlutý rozehraný,
  červený zamčený); vybraný kámen kamera přiletí a level začne. **R** na kameni pustí nejlepší replay toho levelu.
- **Replaye** (29f): nejlepší průlet každého levelu a režimu se ukládá sám; **F5** uloží poslední level (v titulku
  dalšího levelu body a čas, „NEW BEST“; i na kartě konce hry). Menu **Replays**: sledovat (banner REPLAY, Esc
  konec), **C** zkopíruje text `UGHR1:...` do schránky (sdílení), **V** vloží cizí ze schránky, Delete 2× smaže,
  **O** otevře složku.
- **Duch** (29g): poloprůhledný vrtulník nejlepšího průletu letí s tebou; vypnout Settings > Ghost of the best.
- **Pocit z nárazu** (29c): otřes kamery (Settings > Game > Camera shake) a vibrace gamepadu - havárie, kámen na
  nepříteli, pád do moře, náraz do okraje; v týmu vibruje jen pad toho pilota.
- **Zvuk prostředí** (29b): moře, džungle, vítr a déšť v bouři, ohně, šplouchání, liány; hlasitost Settings >
  Ambience. Agent ho nikdy neslyšel - poslech je na tobě.
- **Varování** (27b): vykřičníky nad vrtulníkem při nebezpečné rychlosti; rozmazání rychlého letu (27).
- **Živí cestující** (29d): přešlapují, mávají, krčí se před nízko letícím vrtulníkem, po doručení radost.
- **Grafika** (29a, 30a-30e): viditelné obláčky efektů, vrstvy skály, tráva přes hrany a cestičky k jeskyním, opar
  a paprsky v jeskyních, mokrá skála u hladiny a odraz levelu v moři (i na Low), stín vrtulníku pod ním.

## Co vyzkoušet nového (krok 32, zip 2026-10-10)

- **Rotor** (32a): listy už nejsou z jedné strany náboje černé (osvětlené i zespodu).
- **Tmavší obraz na Low** (32b): Low dřív mléčný (noci a bouře přesvícené), teď blízko Epic - sedí jas noci a bouře?
- **HUD** (32h): u každého pilota bublina s cílovou plošinou (čárky jako v originálu) a jízdné ve čtyřech číslicích,
  které klesá.
- **Rozlišitelní cestující** (32c): muž černovlasý, široký (tmavě olivové listy); žena světlá blondýna, štíhlá (červené
  listy); stařec bělovlasý, shrbený, o holi (slámové listy). Pozor: stařec ve vodě vydrží jen 2 s a zachránit nejde,
  utopí se (muž 10 s, žena 20 s) - jako originál.
- **Foukač** (32d): T-rex velký jako v originálu (natočený ke kameře); **ohňostroj** po dokončení levelu větší a delší.
- **Kámen** (32e): ostrý kus břidlice místo hroudy; visí pod vrtulníkem na laně a nikdy nezajede do skály - při
  přistání si sedne vedle na zem.
- **Let mezi levely** (32f): místo střihu do černé 9 s let dronem nad souostrovím od kamene ke kameni přes mlhu;
  klávesa ho urychlí až od titulku dalšího levelu. Trhá někde (hlavně v mlze)?
- **Strom z džungle** (32g): škrtič s kořeny po čele skály dolů, liány, velké listy taro; tvář a chování jako dřív.

## Co čeká na tvoje posouzení (kroky 13-30)

Všechno v okně (`play.ps1`, tj. zabalená hra, nebo `UghGame.exe` ze zipu):

1. **Zvuk** (13): poslech menu, levelu s letcem a foukačem, prohry, hlasitosti (PgUp/PgDn).
2. **Vrtulník a postavy** (16, 17, 19b): vrtulník v pohybu, chůze, mávání, plavání; listy a vlasy MetaHumanů - stačí
   pás listů u ženy (z boku je vidět šedá podprsenka textury)?
3. **Skála** (18b, 18c, 19c): šedý vápenec čela a vchody do jeskyní; dřevěné rámy dveří kresby jsou zatím jen skalní
   oblouk.
4. **Moře, déšť, nálady** (19): jak tmavá smí být noc a jak oranžový soumrak (`UghMood.cpp`).
5. **Cedule a bubliny** (19d): má zobáček bubliny mířit k vrtulníku (originál ne, `UghBubbles::Place`)?
6. **Let ke kameni** (19e): rychlost a délka (`UghIntro.cpp`), tvar a velikost kamene, obloha a expozice venku, zda
   má být kolem levelu vidět víc kamene (`ScreenMargin`).
7. **Vodopády** (19f): šířka potoka, v kolika levelech, mlha u paty.
8. **Oheň a louče** (19g): stačí plamen ze simulace (jinak oheň z Fabu)? Jas světel; v noci horní louče silně
   prosvětlují strop.
9. **Nepřátelé** (19h): tvář stromu, barva kůže triceratopse, prach z nozder T-rexe, zuby v koutku tlamy; licence
   triceratopse ze Sketchfabu (zapsaná jen jako „stažený Janem“).
10. **Efekty** (20): výbuch, síla prachu a poryvu foukače, stoupající body.
11. **Menu a HUD** (21): logo, barvy a velikosti, let kamery v menu.
12. **Ovládání a nastavení** (22): skutečný gamepad (Xbox) a dva gamepady v týmu, přepnutí rozlišení a okna, jak
    vypadají Low a Medium.
13. **Vydání** (23): spustit zip na RTX 5060 Ti, ověřit automatickou volbu Epic + DLSS, frame generation 2x-4x,
    plynulost prvního letu (PSO cache).
14. **Výkon na notebooku** (24f): zahrát `play.ps1` na Low - je to plynulé? Vzhled Low: bez tmavé svatozáře kolem
    postav, moře bez odrazů enginu (dřív bílé), rozlišení se samo snižuje, když notebook nestíhá. Medium: ohně bez
    stínů.
15. **Menší balíček** (28): zip je teď Shipping build (na notebooku ověřený jen s FSR - snímky stejné jako
    Development); doma na RTX 5060 Ti ověřit DLSS, frame generation 2x-4x a že textury nikde nevypadají měkčeji.
16. **Kroky 29-30** (zip po krocích 29-30, viz Co vyzkoušet nového): poslech ambience a její hlasitosti, vibrace dvou
    padů, síla otřesu, duch, replay přenesený textem mezi notebookem a domem. Na Low drobné tyrkysové tečky na vodě
    (odlesky, ne odraz 30d) - vadí?
