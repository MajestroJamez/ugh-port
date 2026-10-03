// How a frame ended.
#pragma once

namespace ugh::game {

/** After a frame: the game goes on, or it is over. */
enum class GameResult { Continue, GameOver, AllLevelsDone };

}  // namespace ugh::game
