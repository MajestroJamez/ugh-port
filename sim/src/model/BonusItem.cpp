#include "model/BonusItem.hpp"

#include "bonuses/BonusState.hpp"

namespace ugh::model {

void BonusItem::spawn(const data::BonusKind& kind, core::Fixed x, core::Fixed y, core::Fixed vx, core::Word lift) {
    s_.kind = &kind;
    s_.x = x - core::Fixed::fromPixels(kind.x);
    s_.y = y - core::Fixed(kind.y << 4);   // half its height
    s_.timer.setSpeedX(vx);
    s_.vy = -(lift + kind.lift);
    s_.sprite = kind.sprite;
}

void BonusItem::update(Level& level) { s_.state->update(*this, level); }

void BonusItem::changeState(const bonuses::BonusState& next, Level& level) {
    s_.state = &next;
    next.enter(*this, level);
}

}  // namespace ugh::model
