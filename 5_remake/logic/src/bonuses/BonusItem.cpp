#include "bonuses/BonusItem.hpp"

#include "bonuses/BonusState.hpp"
#include "bonuses/Falling.hpp"

namespace ugh::bonuses {

BonusItem::BonusItem(int slot, const data::BonusKind& kind, units::Fixed x, units::Fixed y, units::Fixed speedX,
                     units::Int16 lift)
    : StateMachine(Falling::instance),
      slot_(slot),
      kind_(&kind),
      x_(x - units::Fixed::fromPixels(kind.anchorX)),
      y_(y - units::Fixed::fromRaw(kind.anchorY << 4)),   // half its height
      speedX_(speedX),
      fallSpeed_(-(lift + kind.lift)) {}

void BonusItem::moveTo(units::Fixed x, units::Fixed y, units::Int16 fallSpeed) {
    x_ = x;
    y_ = y;
    fallSpeed_ = fallSpeed;
}

}  // namespace ugh::bonuses
