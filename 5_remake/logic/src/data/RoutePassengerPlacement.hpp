// A passenger with a route in the definition of a level.
#pragma once

#include <utility>

#include "data/PassengerKind.hpp"
#include "data/PassengerPlacement.hpp"
#include "data/Route.hpp"

namespace ugh::data {

/** A passenger that rides its route from pad to pad. */
class RoutePassengerPlacement : public PassengerPlacement {
public:
    RoutePassengerPlacement(const PassengerKind& kind, Route route) : kind_(&kind), route_(std::move(route)) {}

    const PassengerKind& kind() const { return *kind_; }
    const Route& route() const { return route_; }

    void accept(PassengerPlacementVisitor& visitor) const override { visitor.visit(*this); }

private:
    const PassengerKind* kind_;
    Route route_;
};

}  // namespace ugh::data
