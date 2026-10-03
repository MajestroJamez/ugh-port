// Makes the enemies of a level.
#pragma once

#include "data/EnemyPlacementVisitor.hpp"
#include "data/GameData.hpp"
#include "enemies/Enemies.hpp"

namespace ugh::enemies {

/** Makes the right enemy of each placement of a level (Factory, as a Visitor of the placements). */
class EnemyFactory : public data::EnemyPlacementVisitor {
public:
    EnemyFactory(Enemies& enemies, const data::GameData& data) : enemies_(enemies), data_(data) {}

    void visit(const data::FlyerPlacement& placement) override;
    void visit(const data::WalkerPlacement& placement) override;
    void visit(const data::BlowerPlacement& placement) override;
    void visit(const data::TreePlacement& placement) override;

private:
    Enemies& enemies_;
    const data::GameData& data_;
};

}  // namespace ugh::enemies
