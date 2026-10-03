#include "physics/TouchBox.hpp"

namespace ugh::physics {

namespace {

using units::Fixed;
using world::copter::CopterShape;

}  // namespace

TouchBox::TouchBox(const data::kinds::Box& box, Fixed x, Fixed y)
    : left_(x + Fixed::fromPixels(box.x - box.halfWidth) - Fixed::fromPixels(CopterShape::BODY_RIGHT)),
      right_(x + Fixed::fromPixels(box.x + box.halfWidth) - Fixed::fromPixels(CopterShape::BODY_LEFT)),
      top_(y + Fixed::fromPixels(box.y - 2 * box.halfHeight) - Fixed::fromPixels(CopterShape::BODY_HEIGHT)),
      bottom_(y + Fixed::fromPixels(box.y)) {}

TouchBox TouchBox::between(Fixed left, Fixed right, Fixed top, Fixed bottom) { return {left, right, top, bottom}; }

bool TouchBox::touches(const world::copter::Copter& copter) const {
    return bottom_ >= copter.motion().y() && top_ <= copter.motion().y() && right_ >= copter.motion().x() && left_ <= copter.motion().x();
}

world::copter::Copter* TouchBox::firstCopterIn(world::copter::Copters& copters) const {
    for (world::copter::Copter& copter : copters.all())
        if (touches(copter)) return &copter;
    return nullptr;
}

}  // namespace ugh::physics
