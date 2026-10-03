// The tree.
#pragma once

#include <vector>

#include "data/BonusKind.hpp"
#include "data/TreeKind.hpp"
#include "data/TreePlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/tree/TreeState.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"

namespace ugh::enemies::tree {

/** The tree: a passenger falling onto it bounces off half as high and shakes the tree's next bonus item out of it. */
class Tree : public Enemy, public state::StateMachine<Tree, EnemyContext> {
public:
    Tree(int index, const data::TreeKind& kind, const data::TreePlacement& placement);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::TreeKind& kind() const { return *kind_; }

    /** The bonus item it drops next (its index in the tree's list). */
    int nextDrop() const { return nextDrop_; }
    bool hasDrops() const { return nextDrop_ < static_cast<int>(drops_->size()); }
    /** Takes the next bonus item out of the tree. */
    const data::BonusKind& takeDrop() { return *(*drops_)[nextDrop_++]; }

    /** How long it rests after a passenger bounced off it. */
    units::Countdown& restTime() { return restTime_; }
    const units::Countdown& restTime() const { return restTime_; }

private:
    const data::TreeKind* kind_;
    const std::vector<const data::BonusKind*>* drops_;
    int nextDrop_ = 0;
    units::Countdown restTime_;
};

}  // namespace ugh::enemies::tree
