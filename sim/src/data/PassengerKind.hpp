// A kind of passenger.
#pragma once

#include <cstdint>

#include "core/Word.hpp"
#include "data/Animation.hpp"
#include "data/AnimationPair.hpp"
#include "data/Box.hpp"

namespace ugh::data {

/** A kind of passenger (descriptor at 7720, 776a, 77b4 ... 0x4a bytes each). */
struct PassengerKind {
    /**
     * Which state machine the kind belongs to (they share the state handler slots of the descriptor):
     * a walking passenger rides between the pads of its route, it turns into its swimming kind in the water, and
     * the standing passenger waits on its pad to be carried anywhere.
     */
    enum class Type { Walking, Swimming, Standing };

    uint16_t origin = 0;                    // the offset in the original's data (its identity in the replays)
    Type type = Type::Walking;
    Box box;
    const Animation* standing = nullptr;    // +2e (swimming: treading water)
    const Animation* waving = nullptr;      // +30
    AnimationPair walking;                  // +32 / +34
    const Animation* appearing = nullptr;   // +36: out of the door
    const Animation* entering = nullptr;    // +38: into the door
    core::Word animDelay;                   // +3e: frames per animation step
    core::Word fare, fareMin;               // +40 / +42: the fare when boarding, it drops to the minimum
    core::Word swimTime;                    // +44: frames a swimmer stays afloat
    core::Word look;                        // +46: who sits in the copter (Copter::carrying)
    const PassengerKind* other = nullptr;   // +48: the swimming kind of a walking one and back
    bool rescuable = true;                  // a swimmer a copter on the water can pick up (all but 77fe)
};

}  // namespace ugh::data
