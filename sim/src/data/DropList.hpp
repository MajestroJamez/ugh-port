// The bonus items a tree drops.
#pragma once

#include <cstdint>
#include <vector>

#include "data/BonusKind.hpp"

namespace ugh::data {

/** The bonus items a tree drops, one per passenger that bounces off it (level list C). */
struct DropList {
    uint16_t origin = 0;   // the offset in the original's data
    std::vector<const BonusKind*> items;
};

/** The next bonus item of a tree. */
struct DropCursor {
    const DropList* list = nullptr;
    int index = 0;

    bool finished() const { return index >= static_cast<int>(list->items.size()); }
    const BonusKind& next() const { return *list->items[index]; }
};

}  // namespace ugh::data
