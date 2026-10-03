// The fields of an enemy in the replay state (object.N.*).
#pragma once

#include "data/GameData.hpp"
#include "model/Enemy.hpp"
#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

class EnemyFields {
public:
    /** `data` finds the kinds, animations and drop lists. */
    static void visit(FieldVisitor& v, model::Enemy::Snapshot& s, const data::GameData& data);
};

}  // namespace ugh::replay
