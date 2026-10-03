// The passengers' state machines (passengers.cpp).
#pragma once

#include <vector>

namespace ugh {

class PassengerTurn;
struct Passenger;

/** A state a passenger is in from frame to frame; the original keeps the address of its handler (2a0d). */
struct PassengerState {
    const char* name;   // as in the golden replays
    void (*update)(PassengerTurn& turn);
};

/** Where the level load starts the passengers: a walking one at the first stop of its route, the standing one. */
extern const PassengerState NextStop, StartStanding;

/** Every state, for the replay projection. */
const std::vector<const PassengerState*>& passengerStates();

/**
 * A standing passenger dropped from a copter on its way down (113b:2196 checks the descriptor 78dc; its twin
 * 7926 never falls).
 */
bool fallingDown(const Passenger& passenger);

}  // namespace ugh
