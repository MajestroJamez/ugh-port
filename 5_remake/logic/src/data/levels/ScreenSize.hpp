// The size of a level's screen.
#pragma once

namespace ugh::data::levels {

/** A level is one screen of play, 320 x 192 px: its collision mask, the edges of the play, the rain. */
struct ScreenSize {
    static constexpr int WIDTH = 320, HEIGHT = 192;
};

}  // namespace ugh::data::levels
