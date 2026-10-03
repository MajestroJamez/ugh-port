#include "enemies/EnemyBehavior.hpp"

#include "enemies/blower/BlowerBehavior.hpp"
#include "enemies/flyer/FlyerBehavior.hpp"
#include "enemies/tree/TreeBehavior.hpp"
#include "enemies/walker/WalkerBehavior.hpp"

namespace ugh::enemies {

const EnemyBehavior& EnemyBehavior::of(data::EnemyKind::Type type) {
    switch (type) {
        case data::EnemyKind::Type::Flyer: return FlyerBehavior::instance;
        case data::EnemyKind::Type::Blower: return BlowerBehavior::instance;
        case data::EnemyKind::Type::Tree: return TreeBehavior::instance;
        case data::EnemyKind::Type::Walker: break;
    }
    return WalkerBehavior::instance;
}

}  // namespace ugh::enemies
