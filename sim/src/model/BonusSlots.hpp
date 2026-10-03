// The 12 bonus items.
#pragma once

#include <array>

#include "model/BonusItem.hpp"

namespace ugh::model {

class Level;

/** The slots of the bonus items; a dropped item takes a free one, the last first. */
class BonusSlots {
public:
    static constexpr int SLOTS = 12;

    BonusSlots();

    BonusItem& operator[](int slot) { return items_[slot]; }
    const BonusItem& operator[](int slot) const { return items_[slot]; }

    /** 113b:2b96 - the free slot a dropped item takes (the last first); nullptr when all are in use. */
    BonusItem* freeSlot();

    /** 113b:2b7f - Bonuses.kt bonusesUpdate: every item in use, the last slot first. */
    void update(Level& level);

    /** All slots free (113b:0b4f: nothing is drawn yet). */
    void clear();

private:
    std::array<BonusItem, SLOTS> items_;
};

}  // namespace ugh::model
