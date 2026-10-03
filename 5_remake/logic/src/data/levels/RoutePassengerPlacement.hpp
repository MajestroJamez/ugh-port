// A passenger with a route in the definition of a level.
#pragma once

#include <utility>

#include "data/kinds/RoutePassengerKind.hpp"
#include "data/levels/PassengerPlacement.hpp"
#include "data/levels/Route.hpp"

namespace ugh::data::levels {

/** A passenger that rides its route from pad to pad. */
struct RoutePassengerPlacement : PassengerPlacement {
    RoutePassengerPlacement(const kinds::RoutePassengerKind& itsKind, Route itsRoute)
        : kind(&itsKind), route(std::move(itsRoute)) {}
    void accept(PassengerPlacementVisitor& visitor) const override { visitor.visit(*this); }

    const kinds::RoutePassengerKind* kind;
    Route route;
};

}  // namespace ugh::data::levels
