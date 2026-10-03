// The level data comes from the Amiga version of the game; the PC version scales it when it loads a level.
#pragma once

#include "core/Word.hpp"

namespace ugh::core {

/** 113b:3d4d - Amiga rows to PC rows (256 to 192 lines): y - (y >> 2). */
constexpr Word amigaRowsToPc(Word y) { return y - (y >> 2); }

/** 113b:3d5a - Amiga frames to PC frames (50 to 70 Hz, roughly): frames + (frames >> 1). */
constexpr Word amigaFramesToPc(Word frames) { return frames + (frames >> 1); }

}  // namespace ugh::core
