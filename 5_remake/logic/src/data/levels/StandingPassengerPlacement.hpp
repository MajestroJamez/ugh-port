// The standing passenger in the definition of a level.
#pragma once

#include "data/kinds/StandingPassengerKind.hpp"
#include "data/levels/PassengerPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data::levels {

/** The standing passenger: it waits at x, y to be carried anywhere. */
struct StandingPassengerPlacement : PassengerPlacement {
    StandingPassengerPlacement(const kinds::StandingPassengerKind& itsKind, units::Fixed atX, units::Fixed atY)
        : kind(&itsKind), x(atX), y(atY) {}
    void accept(PassengerPlacementVisitor& visitor) const override { visitor.visit(*this); }

    const kinds::StandingPassengerKind* kind;
    units::Fixed x, y;
};

}  // namespace ugh::data::levels
