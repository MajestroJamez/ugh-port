// Makes the enemies of a level.
#pragma once

#include "data/GameData.hpp"
#include "data/levels/EnemyPlacementVisitor.hpp"
#include "enemies/Enemies.hpp"
#include "world/Level.hpp"

namespace ugh::enemies {

/** Makes the right enemy of each placement of a level (Factory, as a Visitor of the placements). */
class EnemyFactory : public data::levels::EnemyPlacementVisitor {
public:
    EnemyFactory(Enemies& enemies, const data::GameData& data, world::Level& level)
        : enemies_(enemies), data_(data), level_(level) {}

    void visit(const data::levels::FlyerPlacement& placement) override;
    void visit(const data::levels::WalkerPlacement& placement) override;
    void visit(const data::levels::BlowerPlacement& placement) override;
    void visit(const data::levels::TreePlacement& placement) override;

private:
    Enemies& enemies_;
    const data::GameData& data_;
    world::Level& level_;
};

}  // namespace ugh::enemies
