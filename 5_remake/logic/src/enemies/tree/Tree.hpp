// The tree.
#pragma once

#include <vector>

#include "data/kinds/BonusKind.hpp"
#include "data/kinds/TreeKind.hpp"
#include "data/levels/TreePlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/tree/TreeState.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"

namespace ugh::enemies::tree {

/** The tree: a passenger falling onto it bounces off half as high and shakes the tree's next bonus item out of it. */
class Tree : public Enemy, public state::StateMachine<Tree, EnemyContext> {
public:
    Tree(int index, const data::kinds::TreeKind& kind, const data::levels::TreePlacement& placement);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::kinds::TreeKind& kind() const { return *kind_; }

    /** The bonus item it drops next (its index in the tree's list). */
    int nextDrop() const { return nextDrop_; }
    bool hasDrops() const { return nextDrop_ < static_cast<int>(drops_->size()); }
    /** Takes the next bonus item out of the tree. */
    const data::kinds::BonusKind& takeDrop() { return *(*drops_)[nextDrop_++]; }

    /** It rests for `frames` after a passenger bounced off it. */
    void startResting(int frames) { restTime_.start(frames); }
    /** One frame of rest; true when it is over. */
    bool tickRest() { return restTime_.tick(); }
    int restTime() const { return restTime_.remaining(); }

private:
    const data::kinds::TreeKind* kind_;
    const std::vector<const data::kinds::BonusKind*>* drops_;
    int nextDrop_ = 0;
    units::Countdown restTime_;
};

}  // namespace ugh::enemies::tree
