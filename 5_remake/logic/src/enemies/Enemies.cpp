#include "enemies/Enemies.hpp"

#include "enemies/EnemyFactory.hpp"

namespace ugh::enemies {

void Enemies::load(const data::levels::LevelDefinition& definition, const data::GameData& data, world::Level& level) {
    all_.clear();
    EnemyFactory factory(*this, data, level);
    for (const auto& placement : definition.enemies) placement->accept(factory);
}

void Enemies::update(const EnemyContext& context) {
    for (auto& enemy : all_) enemy->update(context);
}

void Enemies::hideAll() {
    for (auto& enemy : all_) enemy->hide();
}

}  // namespace ugh::enemies
