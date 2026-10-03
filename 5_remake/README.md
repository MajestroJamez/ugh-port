# 5 - Remake

| Folder | Content |
|---|---|
| `logic/` | the game logic in C++20 without dependencies: a library with a C API and its unit tests (`logic/README.md`) |
| `game/` | the Unreal Engine project (step 10 of `docs/plan.md`), it will take the logic as a module from `../logic` |

The logic knows nothing of the replays; `6_verification/` checks it against them.
