# The replay of a level: `.ughr` (UGHR 1)

A player's replay of one level of the remake, small enough to paste into a chat. The game keeps the best one of each
level and mode by itself, saves the last level's on F5 and plays any of them again (`5_remake/game/README.md`). The
logic writes and reads it (`5_remake/logic/src/record/`, C API `ugh_replay_*` in `ugh_logic.h`); the golden replays
`UGR 1` of `6_verification` are another thing (the original's state, frame by frame, for the tests).

## What a replay is

The logic is deterministic: the same start and the same keys between the same steps give the same game. A replay
therefore keeps only

- **what the level's first attempt started from** (`game::AttemptStart`): the mode (players), the difficulty, the level,
  the lives, the score so far, the multiplier, the state of the random numbers (4 words), the row the rain stops at, how
  hard each pilot last worked his rotor (it turns on with it while the level fades in) and the last key the game loop
  saw (Esc gives up). Everything else of an attempt is loaded from the level's definition. A game resumed from it
  (`ugh_logic_resume_game`) plays the attempt as the game it came from - a level in the middle of a game too;
- **the inputs between the steps** exactly as the logic got them (`ugh_logic_key`, `ugh_logic_menu_key`), from the step
  the first attempt started in (its caption fades in) to the step the level ended in (the next level's attempt starts
  in it, or the game ends) - several attempts when the pilot crashed;
- how it went (steps, the steps of the play - its time -, attempts, the points earned in the level, done or not) and a
  label (the level's password, the pilots' names, the date).

Playing it (`ugh_logic_watch`) resumes a game at the start and gives the logic the recorded inputs before each step:
the same score and the same state every step (CTest `a_replay_of_a_level_plays_again_as_it_was_played`, and
`6_verification`: every attempt of the golden replays after the test pilot's last intervention, cut out, written as text,
read back and played on a resumed game - field by field the golden state). A different logic (`UGH_LOGIC_VERSION`) or
different game data (`ugh-data.ugd`, its CRC-32) may play it differently: the header says which made it, the game warns.

## The file

Binary, little-endian; numbers marked *n* are unsigned LEB128 (7 bits a byte, the low ones first, the high bit "more"),
*z* a signed number zigzagged into one (0, -1, 1, -2 ... as 0, 1, 2, 3 ...), *text* an *n* length and that many bytes of
UTF-8 (at most 32, nothing below a space).

| Field | Type | |
|---|---|---|
| magic | 4 bytes | `UGHR` |
| format | byte | 1 (a reader refuses a newer one: "a newer version of the game made it") |
| logic version | *n* | `UGH_LOGIC_VERSION` of the logic that made it |
| data hash | 4 bytes | CRC-32 of the game data file it was made with |
| players | byte | 1, or 2 for the team |
| difficulty | byte | 0 easy, 1 medium, 2 hard |
| level | *n* | from 0 in the order of the mode |
| lives | *n* | at the start, 0 .. 99 |
| score | *n* | the points before the level |
| multiplier | *n* | 1 .. |
| random | 4 × 2 bytes | the 4 words of the random numbers, word 0 first |
| rain row | byte | where the raindrops stop, 0 .. 255 |
| effort | *z* each player | how hard each pilot last worked his rotor |
| last menu key | byte | 0 Esc, 1 P, 2 any other |
| steps | *n* | from the first attempt's step to the level's last step, 1 .. 2 520 000 (10 hours) |
| play steps | *n* | of them the play (not the captions): its time, at 70.086 steps a second |
| attempts | *n* | 1 .. |
| points | *n* | earned in the level |
| done | byte | 1: the level was done; 0: the game ended in it |
| date | *n* | seconds since 1970 (UTC), 0 unknown |
| password | *text* | the level's |
| names | byte, then *text* each | 0 .. 2 pilots' names |
| inputs | *n*, then *n* each | the count, then each input as one number (below) |
| checksum | 4 bytes | CRC-32 (zip's: IEEE, reflected, 0xEDB88320) of every byte before it |

An **input** is `gap << 6 | code`: `gap` the steps since the input before (the first: since step 0) - the run of steps
without a change of the keys - and `code`:

| Code | Input after the step |
|---|---|
| 0 .. 19 | a pilot's key: `player × 10 + key × 2 + pressed` (key 0 up, 1 down, 2 left, 3 right, 4 fire) |
| 20 .. 39 | the same key, then a menu key "any other" between the same steps (a frontend tells the game loop of every key event: one byte less) |
| 40, 41, 42 | a menu key: Esc, P, any other |

An input "after step *s*" reaches the logic between steps *s* and *s* + 1 (step 0 starts the attempt). A gap below 2
makes an input one byte, below 256 two: a level of two minutes with 150 keys pressed and released is about 650 bytes.

## The text

To share a replay by copy and paste: `UGHR1:` (the format) and the bytes of the file in Base64Url (RFC 4648 section 5:
`A-Z a-z 0-9 - _`, no padding). A reader skips white space around and inside it (a mail breaks long lines) and a byte
order mark; a file may hold the text instead of the bytes. For example a hover of 6 seconds in level 6 of one player on
hard (176 characters):

```
UGHR1:VUdIUgEBMQvntwECBQMAASDKbun_Jd8ItAACugSkAwEAANKAo9YGDkRSRUFNRk9STU9USEVSACXqBCrqByqqDSrqDCrqCCrqCiqqDSrVR5QZ6DWoA6gDqAOoA-gDqAPoA6gDqAOoA6gDqAOoA-gDqAOoA6gD6AOoA6gDKqQs3Q
```

## Refused

- not a replay (no `UGHR`, a golden replay `UGR 1`, any other text or file): "not a UGH! replay";
- a newer format (the format byte, the number after `UGHR` of the text): "a newer version of the game made it";
- damaged: the checksum does not match (a byte changed, cut off), a character outside Base64Url, a value out of range
  or bytes left over;
- the logic refuses to watch a replay of a level its data has not.

A replay of another logic version or other data is read and listed, with a warning: it may play differently.
