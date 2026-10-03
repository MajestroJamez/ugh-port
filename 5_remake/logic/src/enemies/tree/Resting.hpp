// The tree's state Resting.
#pragma once

#include "enemies/tree/TreeState.hpp"

namespace ugh::enemies::tree {

/** Shaken by a passenger, it rests; then it sways again, or stays bare when its bonus items are all gone. */
class Resting : public TreeState {
public:
    static const Resting instance;

    const char* name() const override { return "Resting"; }
    void enter(Tree& tree, const EnemyContext& context) const override;
    void update(Tree& tree, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::tree
