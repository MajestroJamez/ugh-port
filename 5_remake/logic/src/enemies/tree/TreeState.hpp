// A state of the tree.
#pragma once

#include "enemies/EnemyContext.hpp"
#include "state/State.hpp"

namespace ugh::enemies::tree {

class Tree;

/** A state of the tree: swaying, resting after a passenger bounced off it, bare when its bonus items are gone. */
using TreeState = state::State<Tree, EnemyContext>;

}  // namespace ugh::enemies::tree
