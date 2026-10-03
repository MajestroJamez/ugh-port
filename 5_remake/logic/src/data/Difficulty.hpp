// The difficulty of a game.
#pragma once

namespace ugh::data {

/** The difficulty chosen in the menu; it sets the crash limit and the highest score multiplier. */
enum class Difficulty { Easy, Medium, Hard };

/** How many difficulties there are (Hard is the last): the rules have a value for each. */
constexpr int DIFFICULTY_COUNT = static_cast<int>(Difficulty::Hard) + 1;

}  // namespace ugh::data
