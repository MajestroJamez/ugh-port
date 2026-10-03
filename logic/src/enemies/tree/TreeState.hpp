// A state of the tree.
#pragma once

#include "enemies/EnemyContext.hpp"

namespace ugh::enemies::tree {

class Tree;

/** A state of the tree (State): swaying, resting after a passenger bounced off it, bare when its bonus items are gone. */
class TreeState {
public:
    virtual ~TreeState() = default;
    virtual const char* name() const = 0;
    virtual void enter(Tree&, const EnemyContext&) const {}
    virtual void update(Tree& tree, const EnemyContext& context) const = 0;
};

}  // namespace ugh::enemies::tree
