// The fields of a bonus item in the replay state (bonus.N.*).
#pragma once

#include "data/GameData.hpp"
#include "model/BonusItem.hpp"
#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

class BonusFields {
public:
    /** `data` finds the kinds by their offsets in the original's data. */
    static void visit(FieldVisitor& v, model::BonusItem::Snapshot& s, const data::GameData& data);
};

}  // namespace ugh::replay
