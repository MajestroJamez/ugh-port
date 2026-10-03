// The sprites of an animation.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ugh::data {

/** An animation: the sprites of its frames, shown one after another. */
class Animation {
public:
    Animation(std::string name, std::vector<int> frames) : name_(std::move(name)), frames_(std::move(frames)) {}

    const std::string& name() const { return name_; }

    /** The sprite of frame `index` (0 .. length() - 1). */
    int frame(int index) const { return frames_[index]; }
    int length() const { return static_cast<int>(frames_.size()); }

    /** Frame `index` is one past the last: the animation is over (or starts again). */
    bool endsAt(int index) const { return index >= length(); }

private:
    std::string name_;
    std::vector<int> frames_;
};

}  // namespace ugh::data
