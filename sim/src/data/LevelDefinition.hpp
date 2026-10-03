// A level as the data defines it.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "core/Fixed.hpp"
#include "core/Word.hpp"
#include "data/CollisionMask.hpp"
#include "data/EnemyPlacement.hpp"
#include "data/PadDefinition.hpp"
#include "data/PassengerPlacement.hpp"

namespace ugh::data {

/** A level (record of 26 bytes, tables 3349 one player / 33d5 team; 113b:3976 loads it). */
struct LevelDefinition {
    uint16_t origin = 0;                       // the offset of the record in the original's data
    std::vector<PadDefinition> pads;           // list A
    std::vector<PassengerPlacement> passengers;   // list B
    std::vector<EnemyPlacement> enemies;       // list C
    uint8_t toDeliver = 0;                     // +08: passengers to deliver
    uint8_t wind = 0;                          // +0c: 0 none, 1 to the left, 2 to the right; rain with it
    std::array<core::Fixed, 2> startX, startY;   // +0d .. +13: the copters
    core::Fixed water;                         // +15: the water surface
    core::Word waterSpeed;                     // +17: added to the surface every second frame
    CollisionMask mask;
};

}  // namespace ugh::data
