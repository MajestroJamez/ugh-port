// The tree's state Swaying.
#pragma once

#include "enemies/tree/TreeState.hpp"

namespace ugh::enemies::tree {

/**
 * It sways; a passenger falling onto it bounces off half as high, and the tree's next bonus item drops where the
 * passenger hit it.
 */
class Swaying : public TreeState {
public:
    static const Swaying instance;

    const char* name() const override { return "Swaying"; }
    void update(Tree& tree, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::tree
