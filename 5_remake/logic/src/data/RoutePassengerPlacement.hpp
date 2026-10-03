// A passenger with a route in the definition of a level.
#pragma once

#include <utility>

#include "data/PassengerPlacement.hpp"
#include "data/Route.hpp"
#include "data/RoutePassengerKind.hpp"

namespace ugh::data {

/** A passenger that rides its route from pad to pad. */
struct RoutePassengerPlacement : PassengerPlacement {
    RoutePassengerPlacement(const RoutePassengerKind& itsKind, Route itsRoute)
        : kind(&itsKind), route(std::move(itsRoute)) {}
    void accept(PassengerPlacementVisitor& visitor) const override { visitor.visit(*this); }

    const RoutePassengerKind* kind;
    Route route;
};

}  // namespace ugh::data
