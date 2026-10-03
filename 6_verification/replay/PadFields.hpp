// The fields of a pad.
#pragma once

#include "replay/Fields.hpp"
#include "world/scenery/Pad.hpp"

namespace ugh::replay {

/** The pad.N.* fields: where it is, and who waits on it. */
class PadFields {
public:
    static void write(const world::scenery::Pad& pad, int index, Fields& fields);
};

}  // namespace ugh::replay
