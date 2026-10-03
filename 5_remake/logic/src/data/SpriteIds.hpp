// The sprites the game shows by number.
#pragma once

#include <array>

namespace ugh::data {

/** Sprites the logic shows that belong to no animation of a kind (the numbers of assets/sprites/NNN.png). */
struct SpriteIds {
    int standingPassenger = 0;    // the standing passenger waiting on its pad
    int droppedPassenger = 0;     // the standing passenger falling after a copter let it go
    int bouncedPassenger = 0;     // a falling passenger that bounced off an enemy
    int shakenTree = 0;           // a tree a passenger bounced off
    int firstDestinationBubble = 0;   // the bubble over a calling passenger: this + the target pad's index,
    int lastDestinationBubble = 0;    //   at most this one
    int impatientBubble = 0;
    std::array<int, 2> firstRotor{};   // the rotor of each player's copter: the first and the last sprite
    std::array<int, 2> lastRotor{};
};

}  // namespace ugh::data
