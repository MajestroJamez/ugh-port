#include "physics/TouchBox.hpp"

namespace ugh::physics {

namespace {

using units::Fixed;

// the body of a copter, in pixels from its top left corner: 5 .. 26 across, 20 high
constexpr int COPTER_BODY_LEFT = 5, COPTER_BODY_RIGHT = 26, COPTER_HEIGHT = 20;

}  // namespace

TouchBox::TouchBox(const data::kinds::Box& box, Fixed x, Fixed y)
    : left_(x + Fixed::fromPixels(box.x - box.halfWidth) - Fixed::fromPixels(COPTER_BODY_RIGHT)),
      right_(x + Fixed::fromPixels(box.x + box.halfWidth) - Fixed::fromPixels(COPTER_BODY_LEFT)),
      top_(y + Fixed::fromPixels(box.y - 2 * box.halfHeight) - Fixed::fromPixels(COPTER_HEIGHT)),
      bottom_(y + Fixed::fromPixels(box.y)) {}

TouchBox TouchBox::between(Fixed left, Fixed right, Fixed top, Fixed bottom) { return {left, right, top, bottom}; }

bool TouchBox::touches(const world::Copter& copter) const {
    return bottom_ >= copter.y() && top_ <= copter.y() && right_ >= copter.x() && left_ <= copter.x();
}

world::Copter* TouchBox::firstCopterIn(world::Copters& copters) const {
    for (world::Copter& copter : copters.all())
        if (touches(copter)) return &copter;
    return nullptr;
}

}  // namespace ugh::physics
