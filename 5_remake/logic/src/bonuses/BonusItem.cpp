#include "bonuses/BonusItem.hpp"

#include "bonuses/BonusState.hpp"
#include "bonuses/Falling.hpp"

namespace ugh::bonuses {

using units::Fixed;

BonusItem::BonusItem(int slot, const data::kinds::BonusKind& kind, Fixed x, Fixed y, Fixed speedX, Fixed lift)
    : Figure(slot, x - Fixed::fromPixels(kind.anchorX),
             y - Fixed::fromPixels(kind.anchorY).half()),   // half its height
      StateMachine(Falling::instance),
      kind_(&kind),
      speedX_(speedX),
      fallSpeed_(-(lift + Fixed::fromRaw(kind.lift))) {
    showSprite(kind.sprite);
}

}  // namespace ugh::bonuses
