// An enemy of a level.
#pragma once

#include "core/Fixed.hpp"
#include "core/Word.hpp"
#include "data/DropList.hpp"
#include "data/EnemyKind.hpp"

namespace ugh::data {

/** An enemy of a level (list C); which values a kind has depends on the kind (see the loader). */
struct EnemyPlacement {
    const EnemyKind* kind = nullptr;
    int pad = 0;                       // walker, tree
    core::Fixed x, y;                  // walker, blower, tree
    core::Word startDelay;             // flyer: frames before its first flight
    core::Fixed vx;                    // flyer, walker: speed per frame
    const DropList* drops = nullptr;   // tree
};

}  // namespace ugh::data
