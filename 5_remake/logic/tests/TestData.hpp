// The game data for the tests.
#pragma once

#include "data/GameData.hpp"

namespace ugh::test {

/** The real game data (the path from the command line), read once; the tests fail without it. */
const data::GameData& gameData();

}  // namespace ugh::test
