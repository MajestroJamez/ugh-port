# Mapa `logic/` → Kotlin port → originál

Jediné místo, kde se nové jádro `logic/` (C++) potkává s pamětí a kódem originálu: pro každou třídu a metodu logiky
funkce Kotlin portu (`core/src/main/kotlin/ugh/core/game/`) a adresa rutiny originálu (`113b:xxxx`). V kódu `logic/src`
odkazy na originál nejsou (zásady v [rewrite-design.md](rewrite-design.md), kap. 3). Tabulku vyplňuje krok N8.

| `logic/` | Kotlin port | Originál |
|---|---|---|
