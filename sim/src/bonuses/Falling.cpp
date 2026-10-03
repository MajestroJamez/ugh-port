#include "bonuses/Falling.hpp"

#include "bonuses/Lying.hpp"

namespace ugh::bonuses {

namespace {

using core::Fixed;
using core::Word;

constexpr Word GRAVITY = 3;   // 1/32 px per frame, per frame
constexpr Fixed LEFT_EDGE(-0x200), RIGHT_EDGE(0x2800), BOTTOM_EDGE(0x1800);   // gone past these

}  // namespace

const Falling Falling::instance{};

void Falling::drop(model::Level& level, const data::BonusKind& kind, Fixed x, Fixed y, Fixed vx, Word lift) {
    model::BonusItem* item = level.bonuses().freeSlot();
    if (!item) {
        // the original returns without its POP BX and jumps to CS:BX
        level.diagnostics().report("all 12 bonus slots in use: the original would jump to CS:BX");
        return;
    }
    item->spawn(kind, x, y, vx, lift);
    item->changeState(instance, level);
}

/** 113b:2be4 - flies and falls until it lands on a pad or leaves the screen. */
void Falling::update(model::BonusItem& item, model::Level& level) const {
    const data::BonusKind& kind = item.kind();
    Fixed x = item.x() + item.speedX();
    if (x <= LEFT_EDGE || x >= RIGHT_EDGE) {
        item.disappear();
        return;
    }
    item.moveToX(x);
    Word speed = item.fallSpeed() + GRAVITY;
    item.setFallSpeed(speed);
    if (speed < 0) {   // still on its way up
        item.moveToY(item.y() + Fixed(speed));
        return;
    }
    Fixed before = item.y();
    Fixed y = before + Fixed(speed);
    if (y >= BOTTOM_EDGE) {
        item.disappear();
        return;
    }
    item.moveToY(y);
    Word bottom = y.pixels() + kind.y, bottomBefore = before.pixels() + kind.y;
    Word middle = item.x().pixels() + kind.x;
    for (int i = 0; i < level.padCount(); i++) {
        const model::Pad& pad = level.pad(i);
        // unlike a passenger, an item already on the surface does not land, and it lands one pixel further right
        if (bottomBefore < pad.y() && bottom >= pad.y() && middle >= pad.left() && middle - 1 <= pad.right()) {
            item.moveToY(Fixed::fromPixels(pad.y() - kind.y));
            item.changeState(Lying::instance, level);
            return;
        }
    }
}

}  // namespace ugh::bonuses
