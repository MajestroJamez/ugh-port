// The tree's state Bare.
#pragma once

#include "enemies/tree/TreeState.hpp"

namespace ugh::enemies::tree {

/** Its bonus items are all gone: it stays still. */
class Bare : public TreeState {
public:
    static const Bare instance;

    const char* name() const override { return "Bare"; }
    void update(Tree&, const EnemyContext&) const override {}
};

}  // namespace ugh::enemies::tree
