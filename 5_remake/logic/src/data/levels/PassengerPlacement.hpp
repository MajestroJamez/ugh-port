// A passenger in the definition of a level.
#pragma once

#include "data/levels/PassengerPlacementVisitor.hpp"

namespace ugh::data::levels {

/** A passenger of a level as the data defines it. */
struct PassengerPlacement {
    virtual ~PassengerPlacement() = default;
    virtual void accept(PassengerPlacementVisitor& visitor) const = 0;
};

}  // namespace ugh::data::levels
