// The fields of a copter in the replay state (copter.N.*).
#pragma once

#include "model/Copter.hpp"
#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

class CopterFields {
public:
    static void visit(FieldVisitor& v, model::Copter::Snapshot& s);
};

}  // namespace ugh::replay
