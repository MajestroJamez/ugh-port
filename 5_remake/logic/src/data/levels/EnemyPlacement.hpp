// An enemy in the definition of a level.
#pragma once

#include "data/levels/EnemyPlacementVisitor.hpp"

namespace ugh::data::levels {

/** An enemy of a level as the data defines it. */
struct EnemyPlacement {
    virtual ~EnemyPlacement() = default;
    virtual void accept(EnemyPlacementVisitor& visitor) const = 0;
};

}  // namespace ugh::data::levels
