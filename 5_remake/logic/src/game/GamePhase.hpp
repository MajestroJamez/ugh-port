// Where the game is.
#pragma once

namespace ugh::game {

/** Where the game is, as a frontend (and the replays) see it. */
enum class GamePhase {
    Start,           // a new game, before its first frame
    BetweenLevels,   // the black screen before the first caption
    Caption,         // the caption of a level fades in, waits for a key, fades out
    Setup,           // the black screen before the play
    Play,            // the level is played (also while it fades in and out)
};

}  // namespace ugh::game
