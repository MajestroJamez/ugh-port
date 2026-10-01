First complete version of the port of the 1994 DOS game **UGH!** (Play Byte / Bones Park Software Artistic).

The whole program - game logic and physics, all levels and both player modes, graphics, intro, menus, passwords, cut scenes, high scores, AdLib music and sound effects - is ported to Kotlin and verified frame by frame against the original running in a reference emulator.

## How to play

1. Download `UGH-port-windows.zip` below and unpack it (Windows 10/11, no Java needed - it is included).
2. Get the original game: the copy this port was made from was downloaded from https://mujsoubor.cz/stare-hry/ugh. The port needs exactly this `UGH.EXE`:
   SHA-256 `ef93d2cd5eb558f6a7d0007e0109f2e9ee952dc125389d7a6256646087636d7c`
3. Put `UGH.EXE` next to `UGH-port.exe` (or start `UGH-port.exe` and pick it once; the path is remembered).
4. Start `UGH-port.exe`.

Controls: player 1 arrow keys (up = pedal), player 2 W / Z / A / S. F1 starts, F2 password, F3 difficulty, F4 one player / team, F5 controls, P pause, Esc ends the game, Q quits. Passwords of the original work. The high score table is kept in `%APPDATA%\ugh-port`.

The package contains only the port and a Java runtime - no game data. UGH! is © Play Byte / Bones Park Software Artistic.
