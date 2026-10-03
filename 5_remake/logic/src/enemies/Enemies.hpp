// The enemies of a level.
#pragma once

#include <memory>
#include <vector>

#include "data/GameData.hpp"
#include "data/levels/LevelDefinition.hpp"
#include "enemies/Enemy.hpp"
#include "world/Level.hpp"

namespace ugh::enemies {

/** The enemies of the level, in the order of its definition (the order of their updates). */
class Enemies {
public:
    /** The enemies of a new attempt at `definition`, on the pads of `level`. */
    void load(const data::levels::LevelDefinition& definition, const data::GameData& data, world::Level& level);

    /** Every enemy's state, in order. */
    void update(const EnemyContext& context);
    /** Nothing of them is shown (before the play of an attempt). */
    void hideAll();

    int count() const { return static_cast<int>(all_.size()); }
    const Enemy& operator[](int i) const { return *all_[i]; }

private:
    friend class EnemyFactory;

    std::vector<std::unique_ptr<Enemy>> all_;
};

}  // namespace ugh::enemies
