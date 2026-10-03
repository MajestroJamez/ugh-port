// The sprites of an animation.
#pragma once

#include <string>
#include <vector>

namespace ugh::data::kinds {

/** An animation: the sprites of its frames, shown one after another. */
struct Animation {
    std::string name;
    std::vector<int> frames;

    /** The sprite of frame `index` (0 .. length() - 1). */
    int frame(int index) const { return frames[index]; }
    int length() const { return static_cast<int>(frames.size()); }
    /** Frame `index` is one past the last: the animation is over (or starts again). */
    bool endsAt(int index) const { return index >= length(); }
};

}  // namespace ugh::data::kinds
