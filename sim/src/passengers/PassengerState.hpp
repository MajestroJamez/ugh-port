// A state of the passengers' state machines (State).
#pragma once

#include "model/Level.hpp"
#include "model/Passenger.hpp"

namespace ugh::passengers {

/**
 * A state a passenger is in from one frame to the next; the original keeps the address of its handler (2a0d).
 * The states are stateless singletons (`Waiting::instance`): everything they change is in the passenger and the
 * level. A passenger changes its state with Passenger::changeState (the entry action now, the update from the next
 * frame on) or Passenger::continueIn (both now).
 *
 * Three state machines share the state handler slots of the descriptors (data/GameDataLoader checks them):
 *   walking/   out of the door of the pickup pad, wait, call a copter that lands there, ride, pay, walk to the door
 *              of the target pad, next stop of the route;
 *   swimming/  a walking passenger that fell into the water or was knocked in by a copter: splash, swim, call a
 *              copter on the water or sink;
 *   standing/  the passenger that waits on its pad until a copter picks it up and drops it wherever its pilot wants.
 */
class PassengerState {
public:
    virtual ~PassengerState() = default;

    /** The name of the state in the golden replays. */
    virtual const char* name() const = 0;

    /** What the passenger does when it gets into the state (the original's handler that leads to it). */
    virtual void enter(model::Passenger& passenger, model::Level& level) const;

    /** One frame in the state. */
    virtual void update(model::Passenger& passenger, model::Level& level) const = 0;

    /** The passenger falls down in this state (enemies are hit by it). */
    virtual bool fallsDown() const { return false; }
};

}  // namespace ugh::passengers
