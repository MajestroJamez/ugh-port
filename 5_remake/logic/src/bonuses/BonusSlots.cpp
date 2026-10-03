#include "bonuses/BonusSlots.hpp"

#include "bonuses/BonusState.hpp"

namespace ugh::bonuses {

void BonusSlots::drop(const data::kinds::BonusKind& kind, units::Fixed x, units::Fixed y, units::Fixed speedX, int lift,
                      events::Diagnostics& diagnostics) {
    for (int slot = SLOTS - 1; slot >= 0; slot--) {
        if (items_[slot]) continue;
        items_[slot].emplace(slot, kind, x, y, speedX, lift);
        return;
    }
    diagnostics.report("all 12 bonus slots in use: the original would jump into nowhere");
}

void BonusSlots::update(const world::PlayContext& context) {
    for (int slot = SLOTS - 1; slot >= 0; slot--) {
        if (!items_[slot]) continue;
        items_[slot]->update(context);
        if (items_[slot]->gone()) items_[slot].reset();
    }
}

void BonusSlots::clear() {
    for (auto& item : items_) item.reset();
}

}  // namespace ugh::bonuses
