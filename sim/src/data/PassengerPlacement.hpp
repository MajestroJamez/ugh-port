// A passenger of a level.
#pragma once

#include "core/Fixed.hpp"
#include "data/PassengerKind.hpp"
#include "data/Route.hpp"

namespace ugh::data {

/** A passenger of a level (list B): a walking one with its route, or the standing one on its pad. */
struct PassengerPlacement {
    const PassengerKind* kind = nullptr;
    int pad = 0;              // where it starts
    RouteCursor route;        // the walking passenger
    core::Fixed x, y;         // the standing passenger
};

}  // namespace ugh::data
