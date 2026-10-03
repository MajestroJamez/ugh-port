#include "model/BonusSlots.hpp"

#include <string>

#include "model/Level.hpp"

namespace ugh::model {

BonusSlots::BonusSlots() {
    for (int slot = 0; slot < SLOTS; slot++) items_[slot] = BonusItem(slot);
}

BonusItem* BonusSlots::freeSlot() {
    for (int slot = SLOTS - 1; slot >= 0; slot--)
        if (items_[slot].freeForDrop()) return &items_[slot];
    return nullptr;
}

void BonusSlots::update(Level& level) {
    for (int slot = SLOTS - 1; slot >= 0; slot--) {
        BonusItem& item = items_[slot];
        if (!item.inUse()) continue;
        if (item.snapshot().state && item.snapshot().kind) item.update(level);
        else level.diagnostics().report("bonus item " + std::to_string(slot) + " without a state or kind");
    }
}

void BonusSlots::clear() {
    for (BonusItem& item : items_) item.disappear();
}

}  // namespace ugh::model
