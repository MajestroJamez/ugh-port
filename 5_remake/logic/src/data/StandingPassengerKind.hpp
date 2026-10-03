// A kind of standing passenger.
#pragma once

#include "data/PassengerKind.hpp"

namespace ugh::data {

/** A kind of standing passenger: it waits on its pad to be carried anywhere. */
struct StandingPassengerKind : PassengerKind {
    int look = 0;   // who hangs below the copter
};

}  // namespace ugh::data
