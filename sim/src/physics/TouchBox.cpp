#include "physics/TouchBox.hpp"

namespace ugh::physics {

namespace {

using core::Fixed;

// the body of a copter for touching a sprite, in pixels from its top left corner: 5 .. 26 across, 20 high
constexpr core::Word COPTER_BODY_LEFT = 5, COPTER_BODY_RIGHT = 26, COPTER_HEIGHT = 20;

}  // namespace

TouchBox::TouchBox(const data::Box& box, Fixed x, Fixed y)
    : left_(x + Fixed::fromPixels(box.x - box.halfWidth) - Fixed::fromPixels(COPTER_BODY_RIGHT)),
      right_(x + Fixed::fromPixels(box.x + box.halfWidth) - Fixed::fromPixels(COPTER_BODY_LEFT)),
      top_(y + Fixed::fromPixels(box.y - (box.halfHeight << 1)) - Fixed::fromPixels(COPTER_HEIGHT)),
      bottom_(y + Fixed::fromPixels(box.y)) {}

bool TouchBox::touches(const model::Copter& copter) const {
    return bottom_ >= copter.y() && top_ <= copter.y() && right_ >= copter.x() && left_ <= copter.x();
}

int TouchBox::firstCopterIn(const model::Level& level) const {
    for (int c = 0; c < level.copterCount(); c++)
        if (touches(level.copter(c))) return c;
    return model::Level::NONE;
}

}  // namespace ugh::physics
