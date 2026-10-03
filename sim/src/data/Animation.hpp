// An animation of the data: the sprites of its frames.
#pragma once

#include <cstdint>

#include "data/DataImage.hpp"
#include "data/Sprite.hpp"

namespace ugh::data {

/**
 * Sprite numbers up to the list end. The original reads past the end into whatever follows when an animation
 * position comes from a longer animation, so frame() reads the data image like the original does.
 */
class Animation {
public:
    Animation(const DataImage& image, uint16_t origin) : image_(&image), origin_(origin) {}

    /** The offset of the animation in the original's data (its identity in the replays). */
    uint16_t origin() const { return origin_; }

    Sprite frame(int index) const { return image_->word(origin_ + 2 * index); }

    /** Frame `index` is the end of the list. */
    bool endsAt(int index) const { return frame(index) == DataImage::LIST_END; }

private:
    const DataImage* image_;
    uint16_t origin_;
};

}  // namespace ugh::data
