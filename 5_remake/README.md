# 5 - Remake

| Folder | Content |
|---|---|
| `logic/` | the game logic in C++20 without dependencies: a library with a C API and its unit tests (`logic/README.md`) |
| `game/` | the Unreal Engine 5.8 project: the logic as its module `UghLogic` (the same sources), grey boxes over it, the golden replays as automation tests (`game/README.md`) |

The logic knows nothing of the replays; `6_verification/` checks it against them.
