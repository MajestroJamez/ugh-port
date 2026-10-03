// The fields of a passenger in the replay state (passenger.N.*).
#pragma once

#include "data/GameData.hpp"
#include "model/Passenger.hpp"
#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

class PassengerFields {
public:
    /** `data` finds the kinds and routes by their offsets in the original's data. */
    static void visit(FieldVisitor& v, model::Passenger::Snapshot& s, const data::GameData& data);
};

}  // namespace ugh::replay
