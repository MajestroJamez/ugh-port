// The fields of a pad in the replay state (pad.N.*).
#pragma once

#include "model/Pad.hpp"
#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

class PadFields {
public:
    static void visit(FieldVisitor& v, model::Pad::Snapshot& s);
};

}  // namespace ugh::replay
