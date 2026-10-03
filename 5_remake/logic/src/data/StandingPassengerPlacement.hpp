// The standing passenger in the definition of a level.
#pragma once

#include "data/PassengerKind.hpp"
#include "data/PassengerPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data {

/** The standing passenger: it waits at x, y to be carried anywhere. */
class StandingPassengerPlacement : public PassengerPlacement {
public:
    StandingPassengerPlacement(const PassengerKind& kind, units::Fixed x, units::Fixed y) : kind_(&kind), x_(x), y_(y) {}

    const PassengerKind& kind() const { return *kind_; }
    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }

    void accept(PassengerPlacementVisitor& visitor) const override { visitor.visit(*this); }

private:
    const PassengerKind* kind_;
    units::Fixed x_, y_;
};

}  // namespace ugh::data
