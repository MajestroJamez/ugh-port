// The fields of a copter.
#pragma once

#include "replay/Fields.hpp"
#include "world/copter/Copter.hpp"

namespace ugh::replay {

/** The copter.N.* fields: position, speed, pad, rotor, keys, cargo, fare, effort. */
class CopterFields {
public:
    static void write(const world::copter::Copter& copter, int player, Fields& fields);
};

}  // namespace ugh::replay
