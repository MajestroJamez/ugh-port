// A passenger in the definition of a level.
#pragma once

#include "data/PassengerPlacementVisitor.hpp"

namespace ugh::data {

/** A passenger of a level as the data defines it. */
struct PassengerPlacement {
    virtual ~PassengerPlacement() = default;
    virtual void accept(PassengerPlacementVisitor& visitor) const = 0;
};

}  // namespace ugh::data
