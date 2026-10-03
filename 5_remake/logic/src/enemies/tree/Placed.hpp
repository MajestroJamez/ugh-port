// The tree's state Placed.
#pragma once

#include "enemies/tree/TreeState.hpp"

namespace ugh::enemies::tree {

/** Put into the level, it starts to sway from the first frame. */
class Placed : public TreeState {
public:
    static const Placed instance;

    const char* name() const override { return "Placed"; }
    void update(Tree& tree, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::tree
