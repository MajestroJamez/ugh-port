// The standing passenger in the definition of a level.
#pragma once

#include "data/PassengerPlacement.hpp"
#include "data/StandingPassengerKind.hpp"
#include "units/Fixed.hpp"

namespace ugh::data {

/** The standing passenger: it waits at x, y to be carried anywhere. */
struct StandingPassengerPlacement : PassengerPlacement {
    StandingPassengerPlacement(const StandingPassengerKind& itsKind, units::Fixed atX, units::Fixed atY)
        : kind(&itsKind), x(atX), y(atY) {}
    void accept(PassengerPlacementVisitor& visitor) const override { visitor.visit(*this); }

    const StandingPassengerKind* kind;
    units::Fixed x, y;
};

}  // namespace ugh::data
