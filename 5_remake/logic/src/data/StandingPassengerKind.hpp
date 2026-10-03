// A kind of standing passenger.
#pragma once

#include "data/PassengerKind.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/** A kind of standing passenger: it waits on its pad to be carried anywhere. */
struct StandingPassengerKind : PassengerKind {
    units::Int16 look;   // who hangs below the copter
};

}  // namespace ugh::data
