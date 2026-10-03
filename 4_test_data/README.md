# 4 - Test data

What the remake is built from and checked against, made from the original and the Kotlin port.

| Gradle project | Content |
|---|---|
| `extractor/` (`:extractor`) | `.\gradlew.bat :extractor:run` reads `UGH.EXE` into `assets/`: sprites, palette, levels, pictures, sound blocks and `assets/logic/ugh-data.ugd`, the data of the C++ logic (format in `2_reverse_engineering/notes/phase2-data.md`) |
| `verify/` (`:verify`) | the port against the original, routine by routine and frame by frame; `.\gradlew.bat :verify:replays` records the 161 golden replays `UGR 1` into `verify/build/replays` (format in `docs/rewrite-design.md`, chap. 9) |
