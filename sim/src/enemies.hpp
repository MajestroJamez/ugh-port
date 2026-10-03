// The enemies' state machines and behaviors (enemies.cpp).
#pragma once

#include <vector>

#include "data.hpp"

namespace ugh {

class EnemyTurn;
struct Enemy;

/** A state an enemy is in from frame to frame; the original keeps the address of its handler (2cf3). */
struct EnemyState {
    const char* name;   // as in the golden replays
    void (*update)(EnemyTurn& turn);
};

/**
 * What differs between the kinds of enemies besides their states (Strategy): how the level list puts one into
 * its slot (113b:3b21). A kind sets only the values its entry has; the rest stays from the slot's earlier use.
 */
struct EnemyBehavior {
    void (*place)(Enemy& enemy, const EnemyPlacement& placement);
};

/** The pterodactyl, the triceratops walking on a pad, the blower, the tree that drops bonus items. */
extern const EnemyBehavior Flyer, Walker, Blower, Tree;

/** Every state, for the replay projection. */
const std::vector<const EnemyState*>& enemyStates();

}  // namespace ugh
