// A level as the data defines it.
#pragma once

#include <array>
#include <memory>
#include <vector>

#include "data/CollisionMask.hpp"
#include "data/EnemyPlacement.hpp"
#include "data/PadDefinition.hpp"
#include "data/PassengerPlacement.hpp"
#include "data/Wind.hpp"
#include "units/Fixed.hpp"

namespace ugh::data {

/** A level: its pads, passengers and enemies in their order, the copters' start, the water, the wind, the mask. */
struct LevelDefinition {
    int id = 0;
    int toDeliver = 0;                 // passengers that finish their route before the level is done
    Wind wind = Wind::None;
    std::array<units::Fixed, 2> startX, startY;   // the copters' top left corners
    units::Fixed water;                // the water surface
    units::Fixed waterSpeed;           // added to the surface every second frame (negative: it rises)
    std::vector<PadDefinition> pads;
    std::vector<std::unique_ptr<PassengerPlacement>> passengers;
    std::vector<std::unique_ptr<EnemyPlacement>> enemies;
    CollisionMask mask;
};

}  // namespace ugh::data
