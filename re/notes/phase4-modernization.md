# UGH! - fáze 4: modernizace (UE 5.8), zkušební průchod a golden replays

Stav k 2026-10-02.

## Cíl

Volná 3D re-imaginace UGH! v Unreal Engine 5.8 (2.5D: 3D svět, 2D hratelnost) s fyzikou a logikou
**přesně jako originál**. Kotlin port (`core/`) slouží jen jako reference a generátor testů; nové jádro
bude čisté C++20 bez závislostí na UE, ověřené golden replays. Vizuální směr „Pravěké dioráma“.

Pořadí: (1) sémantický stav + projekce, (2) golden replays, (3) C++ jádro + C++ přehrávač replayů,
(4) UE frontend (šedé kostky, pak vizuál; DLSS/FSR/TSR za přepínačem).

## Toolchain (pracovní notebook)

| Co | Verze / poznámka |
|---|---|
| Unreal Engine | 5.8.3 z Epic Launcheru, se zdrojáky enginu, bez cílových platforem kromě Win64 |
| Kompilátor | VS Build Tools 2026 (18.10.3): MSVC 14.51, Windows SDK 10.0.26100, .NET desktop build tools |
| IDE | JetBrains Rider 2026.2.3 (non-commercial), bez ETW Host Service |
| GPU | Radeon 890M (RDNA 3.5), D3D12 feature level 12_2, ray tracing tier 1.1 |
| Long paths | zapnuté (registr + `git core.longpaths`) |
| DLSS | NVIDIA DLSS 4.5 plugin 8.8.0 (Streamline 2.14.1), DLSS 5 zatím pro UE veřejně není |
| FSR | AMD FSR plugin 4.1.1; na RDNA 3.5 běží FSR 3.1.5 (ML verze FSR 4 jen RX 7000/9000) |

Zkušební projekt: `C:\Users\Ja079591\IdeaProjects\UghTrial` (mimo repo, po průchodu smazat).

## Výsledky zkušebního průchodu

1. Prázdný C++ projekt ze šablony TP_Blank přeložený z příkazové řádky (`Build.bat`, 83 s).
2. Stejná deterministická 16bitová simulace dává bit po bitu stejný výsledek nativně (MSVC),
   z JVM přes FFM (JDK 25) i uvnitř UE jako automatický test (`UnrealEditor-Cmd -nullrhi`, bez okna).
3. Render scény z kódu bez okna (`-game -RenderOffscreen`): Lumen GI + odrazy, HW ray tracing, TSR.
   890M: 18,7 ms/snímek nativně, 13,5 ms s FSR (67 %), 14,1 ms s TSR - vše při 720p.
4. Automatický výběr upscaleru: DLSS (je-li podporováno) → FSR → TSR. Na 890M: DLSS-SR/RR/FG
   „nepodporováno“, zvolí se FSR; vynucené DLSS čistě spadne na TSR.

## Problémy a jejich řešení

- **FSR 4.1.1 pro UE 5.8 padá bez okna**: `FFXFrameInterpolation.cpp` má v 5.8 větvi `check(ViewportRHI)`,
  s `-RenderOffscreen` RHI viewport neexistuje. Lokální oprava (2 místa, „UGH patch“), popsaná
  v `UghTrial/Plugins/PATCHES.md`. Při aktualizaci pluginu aplikovat znovu.
- **Balení (UAT) selhává na Zen serveru**: Zen poslouchá na `[::1]:8558`; když `NO_PROXY` obsahuje
  `localhost,127.0.0.1`, ale ne `::1`, posílá .NET požadavek přes HTTP proxy.
  Řešení: pro proces UAT přidat `$env:NO_PROXY += ",::1,[::1]"`.
- `-ResX/-ResY` se s `-RenderOffscreen` ignoruje (render 1280×720), rozlišení nastavit jinak (`r.SetRes`).
- Zdrojáky pluginů DLSS/FSR mají CRLF, při úpravách skriptem na to myslet.

## Zjištění k originálu důležitá pro C++ jádro

- **Kolize nejsou po dlaždicích, ale po pixelech**: `113b:1457` ORuje 10 pixelů kolizní masky
  (bit 7 barvy) na stránce pozadí ve VGA paměti. C++ jádro potřebuje pixelovou kolizní masku levelu
  (z vykreslených dlaždic), ne jen mapu 20×16.
- Výchozí obtížnost v menu je **MEDIUM** (`[2638] = 1`), F3 cykluje medium → hard → easy.
- Stavy cestujících, objektů a bonusů jsou v originále adresy obslužných rutin; projekce je převádí
  na jména (tabulky v `StateProjection`).
- RNG (`113b:4f09`, 4 slova v CS:4ef7..4efe, seed z vteřin hodin) je součástí stavu.

## Golden replays (formát UGR 0)

Kód: `verify/src/test/kotlin/ugh/verify/replay/` - `StateProjection` (paměť → sémantický stav),
`ReplayWriter`/`ReplayReader` (formát), `GoldenReplayTest` (záznam v lockstepu s originálem).
Soubory: `verify/build/replays/*.ugr`.

- Textový ASCII formát, jeden řádek na snímek: `T <tick> w=<čekání> k=<klávesy> | <pole>=<hodnota> ...`,
  jen změněná pole (delta), zmizelé pole `=~`. Klávesy jsou vstup *dalšího* snímku.
- `I <pole>=<hodnota> ...` = zásah testovacího pilota mezi snímky (cheat pilot přesouvá vrtulník,
  doplňuje energii); přehrávač je nastaví před dalším snímkem.
- Pole: `game.*` (fáze, level, hráči, obtížnost, životy, multiplikátor, skóre, energie, fade, vítr,
  voda, RNG), `copter.N.*`, `pad.N.*`, `passenger.N.*`, `object.N.*`, `bonus.N.*`. Pozice v 1/32 px.
- `game.phase`: `start`, `setup`, `caption`, `play`, `betweenLevels`, ... (z volajícího v portu);
  seznamy levelu jen ve fázích `caption` a `play`.
- Každý snímek se ověřuje, že projekce paměti originálu = projekce portu.
- `B <fáze> <pole>=<hodnota> ...` = co fáze snímku (`passengers`, `objects`, `bonuses`) změnila mimo vlastní
  skupinu polí, s hodnotami před ní. Jádro, které fázi ještě nemá, ji na očekávaném stavu vrátí.

Sada (`.\gradlew.bat :verify:replays`, asi 2 min): 161 záznamů, 434 tisíc snímků. Pro každý level obou režimů je
cheat záznam (start přes heslo), k tomu 4 dlouhé cheat záznamy, ve kterých se dokončují levely, a 7 náhodných
pilotů do game over na všech obtížnostech. Ve všech je 0 rozdílů a jsou pokryté všechny dosažitelné stavy.
Úplnost projekce hlídá `StateAudit` (viz `StateProjection.NOT_PROJECTED`).

## C++ jádro (`sim/`)

C++20 bez závislostí, CMake + Ninja z Build Tools 2026, C API `include/ugh_sim.h` (`ugh_sim_*`). Rozhraní tvoří
pole replayů se stejnými jmény a hodnotami jako v projekci. Uvnitř drží jádro stav jako originál: DGROUP
(64 kB, inicializovaná z `assets/sim/ugh-sim.bin`) na původních adresách, a logika je převedená z Kotlin
portu rutinu po rutině se stejnými pomocníky (`u`, `d`, `setD`, `Regs`). Pole replayů jsou pohled na tuto
paměť. Před každým přechodem jádro doplní skryté proměnné, které replay nemá a logika čte: kopii záznamu
levelu, konce seznamů, volné sloty bonusů, plošiny × 2 a hráče × 2. `tools/ugh_replay.cpp` přehrává `.ugr`
snímek po snímku: vezme zaznamenaný stav před snímkem, I řádek a klávesy, provede přechod (nová hra, start levelu, snímek hry) a porovná známá pole se
stavem po snímku, kde jsou vrácené B řádky fází, které jádro ještě nemá. Protože je projekce úplná (krok 3),
ověřuje přesný přechod každého snímku zvlášť celou simulaci. Úplný běh od začátku přijde v kroku 8.

```powershell
powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\sim\build.ps1
```

Překlad + CTest (jeden test na replay, celkem 7 s). Postup podle kroků je v [plan.md](plan.md).

## Zabalená hra (test DLSS doma)

`RunUAT BuildCookRun` (Development, Win64, mapa `/Engine/Maps/Entry`) → `UghTrial/Packaged/Windows`,
1,4 GB (z toho 383 MB `.pdb`), zip bez `.pdb` 579 MB: `UghTrial/UghTrial-home-test.zip` + `README.txt`.
Ovládání: 1 DLSS, 2 FSR, 3 TSR, G Frame Generation (off/2x/3x/4x), Esc konec; údaje vlevo nahoře.
Bez PSO cache se první sekundy trhá (kompilace shaderů za běhu) - pro hru připravit PSO cache.

## MCP

- **UE 5.8 Unreal MCP** ověřeno: pluginy `ModelContextProtocol` + `AllToolsets` v `.uproject`,
  editor bez okna `UnrealEditor-Cmd <uproject> -nullrhi -ModelContextProtocolStartServer`
  (port `-ModelContextProtocolPort=N`, výchozí 8000), streamable HTTP `http://127.0.0.1:8000/mcp`.
  `tools/list` vrací 3 meta-nástroje (`list_toolsets`, `describe_toolset`, `call_tool`), za nimi
  desítky sad (AutomationTest, ConfigSettings, EditorApp, Logs, PCG, Niagara, ...).
  Pozor: curl musí obejít firemní proxy (`--noproxy '*'`).
- **Licence:** plugin při startu varuje, že data posílaná do LLM jsou „Licensed Technology“ podle
  UE EULA (sekce 6(e)) a poskytovatel LLM je nesmí použít k trénování.
- **Rider MCP**: vestavěný server (Settings → Tools → MCP Server), stejný výchozí port jako IDEA (64342) -
  v Rideru nastavit jiný (např. 64343) a přidat ručně jako `rider`, ne přes Auto-Configure
  (přepsal by záznam `intellij`). Po přidání restart Claude Code.

