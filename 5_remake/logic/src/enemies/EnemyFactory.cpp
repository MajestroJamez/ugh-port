#include "enemies/EnemyFactory.hpp"

#include "enemies/blower/Blower.hpp"
#include "enemies/flyer/Flyer.hpp"
#include "enemies/tree/Tree.hpp"
#include "enemies/walker/Walker.hpp"

namespace ugh::enemies {

void EnemyFactory::visit(const data::levels::FlyerPlacement& placement) {
    enemies_.all_.push_back(std::make_unique<flyer::Flyer>(enemies_.count(), data_.flyerKind(), placement));
}

void EnemyFactory::visit(const data::levels::WalkerPlacement& placement) {
    enemies_.all_.push_back(std::make_unique<walker::Walker>(enemies_.count(), data_.walkerKind(), placement,
                                                                    level_.pad(placement.pad)));
}

void EnemyFactory::visit(const data::levels::BlowerPlacement& placement) {
    enemies_.all_.push_back(std::make_unique<blower::Blower>(enemies_.count(), data_.blowerKind(), placement));
}

void EnemyFactory::visit(const data::levels::TreePlacement& placement) {
    enemies_.all_.push_back(std::make_unique<tree::Tree>(enemies_.count(), data_.treeKind(), placement));
}

}  // namespace ugh::enemies
