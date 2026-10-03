// The copters of a level.
#pragma once

#include <array>
#include <optional>

#include "data/LevelDefinition.hpp"
#include "data/SpriteIds.hpp"
#include "world/Copter.hpp"
#include "world/Water.hpp"

namespace ugh::world {

/** The copters of a level - one, or two in the team mode - and what the entities ask about them. */
class Copters {
public:
    explicit Copters(int count) : count_(count) {}

    int count() const { return count_; }
    Copter& operator[](int player) { return copters_[player]; }
    const Copter& operator[](int player) const { return copters_[player]; }

    /** At the start of an attempt: both copters (also in the one-player mode) at their start, no key held. */
    void placeAtStart(const data::LevelDefinition& definition, const data::SpriteIds& sprites);

    /** The first copter that stands on the pad. */
    std::optional<int> landedOn(int pad) const;
    /** A copter with room for a passenger stands on the pad. */
    bool emptyLandedOn(int pad) const;
    /** The first copter that floats on the water. */
    std::optional<int> onWater(const Water& water) const { return firstOnWater(water, false, false); }
    /** The first copter with room for a passenger that floats on the water. */
    std::optional<int> onWaterWithRoom(const Water& water) const { return firstOnWater(water, true, false); }
    /** The first copter with room for a passenger that floats still on the water (not moving up or down). */
    std::optional<int> stillOnWaterWithRoom(const Water& water) const { return firstOnWater(water, true, true); }

private:
    int count_;
    std::array<Copter, 2> copters_;

    std::optional<int> firstOnWater(const Water& water, bool withRoom, bool still) const;
};

}  // namespace ugh::world
