// A state of a bonus item.
#pragma once

#include "bonuses/BonusItem.hpp"
#include "world/PlayContext.hpp"

namespace ugh::bonuses {

/** A state of a bonus item (State); the states are stateless, the item holds the data. */
class BonusState {
public:
    virtual ~BonusState() = default;
    virtual const char* name() const = 0;
    /** One frame in the state. */
    virtual void update(BonusItem& item, const world::PlayContext& context) const = 0;
};

}  // namespace ugh::bonuses
