// The fields of a bonus item.
#pragma once

#include "bonuses/BonusItem.hpp"
#include "replay/Fields.hpp"

namespace ugh::replay {

/** The bonus.N.* fields of an item in use (N = its slot). */
class BonusFields {
public:
    static void write(const bonuses::BonusItem& item, Fields& fields);
};

}  // namespace ugh::replay
