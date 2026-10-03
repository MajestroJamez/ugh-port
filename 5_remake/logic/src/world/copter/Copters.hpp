// The copters of a level.
#pragma once

#include <array>
#include <span>

#include "data/SpriteIds.hpp"
#include "data/levels/LevelDefinition.hpp"
#include "world/copter/Copter.hpp"
#include "world/scenery/Pad.hpp"
#include "world/scenery/Water.hpp"

namespace ugh::world::copter {

/** The copters of a level - one, or two in the team mode - and what the entities ask about them. */
class Copters {
public:
    explicit Copters(int count) : count_(count) {}

    int count() const { return count_; }
    /** The copter of `player` (0 or 1). */
    Copter& operator[](int player) { return copters_[player]; }
    const Copter& operator[](int player) const { return copters_[player]; }
    /** The copters in play, player 0 first. */
    std::span<Copter> all() { return {copters_.data(), static_cast<size_t>(count_)}; }
    std::span<const Copter> all() const { return {copters_.data(), static_cast<size_t>(count_)}; }

    /** At the start of an attempt: both copters (also in the one-player mode) at their start, no key held. */
    void placeAtStart(const data::levels::LevelDefinition& definition, const data::SpriteIds& sprites);

    /** The first copter that stands on the pad; nullptr if none. */
    Copter* landedOn(const scenery::Pad& pad);
    /** A copter with room for a passenger stands on the pad. */
    bool landedOnWithRoom(const scenery::Pad& pad) const;
    /** The first copter that floats on the water; nullptr if none. */
    Copter* onWater(const scenery::Water& water) { return firstOnWater(water, Wanted::Any); }
    /** The first copter with room for a passenger that floats on the water. */
    Copter* onWaterWithRoom(const scenery::Water& water) { return firstOnWater(water, Wanted::WithRoom); }
    /** The first copter with room for a passenger that floats still on the water (not moving up or down). */
    Copter* stillOnWaterWithRoom(const scenery::Water& water) { return firstOnWater(water, Wanted::StillWithRoom); }

private:
    /** Which copter on the water a swimmer looks for. */
    enum class Wanted { Any, WithRoom, StillWithRoom };

    int count_;
    std::array<Copter, 2> copters_{Copter(0), Copter(1)};

    Copter* firstOnWater(const scenery::Water& water, Wanted wanted);
    /** The copter is what is wanted (wherever it is). */
    static bool fits(const Copter& copter, Wanted wanted);
};

}  // namespace ugh::world::copter
