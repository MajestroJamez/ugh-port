// The tree.
#pragma once

#include "data/Animation.hpp"

namespace ugh::data {

/** The tree: it sways, and a passenger bouncing off it shakes a bonus item out of it. */
struct TreeKind {
    const Animation* swaying = nullptr;
};

}  // namespace ugh::data
