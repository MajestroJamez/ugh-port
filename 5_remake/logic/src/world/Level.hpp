// The world of the level being played.
#pragma once

#include <array>
#include <optional>
#include <vector>

#include "data/GameData.hpp"
#include "data/LevelDefinition.hpp"
#include "events/Diagnostics.hpp"
#include "events/EventListener.hpp"
#include "world/Copter.hpp"
#include "world/Energy.hpp"
#include "world/Fade.hpp"
#include "world/Pad.hpp"
#include "world/Rain.hpp"
#include "world/RandomNumbers.hpp"
#include "world/Water.hpp"

namespace ugh::world {

/**
 * The world of the level being played: the copters, the pads, the water, the rain, the energy and the fade, how
 * many passengers are left; and the questions the entities ask about it (which copter landed on a pad, which one
 * floats on the water). It lasts for the whole game: an attempt reloads it from the level's definition, but the
 * copters keep their effort and the rain keeps its floor row.
 */
class Level {
public:
    explicit Level(int players = 1) : players_(players) {}

    /** A new attempt at `definition`: the copters at their start, the pads, the water, full energy, fading in. */
    void startAttempt(const data::LevelDefinition& definition, const data::SpriteIds& sprites, RandomNumbers& random,
                      events::Diagnostics& diagnostics);

    const data::LevelDefinition* definition() const { return definition_; }
    data::Wind wind() const { return definition_ ? definition_->wind : data::Wind::None; }
    bool windy() const { return wind() != data::Wind::None; }

    int copterCount() const { return players_; }
    Copter& copter(int player) { return copters_[player]; }
    const Copter& copter(int player) const { return copters_[player]; }
    int padCount() const { return static_cast<int>(pads_.size()); }
    Pad& pad(int i) { return pads_[i]; }
    const Pad& pad(int i) const { return pads_[i]; }
    Water& water() { return water_; }
    const Water& water() const { return water_; }
    Rain& rain() { return rain_; }
    const Rain& rain() const { return rain_; }
    Energy& energy() { return energy_; }
    const Energy& energy() const { return energy_; }
    Fade& fade() { return fade_; }
    const Fade& fade() const { return fade_; }

    int passengersLeft() const { return passengersLeft_; }
    bool done() const { return done_; }
    /** A passenger finished its route; the last one ends the level (the count never goes below zero). */
    void passengerFinished(events::EventListener& events);

    // ------------------------------------------------------------ what the entities ask

    /** The background is solid at the pixel. */
    bool solid(int x, int y) const { return definition_ && definition_->mask.solid(x, y); }
    /** The first copter that stands on the pad. */
    std::optional<int> copterLandedOn(int pad) const;
    /** A copter with room for a passenger stands on the pad. */
    bool emptyCopterLandedOn(int pad) const;
    /** The first copter that floats on the water; `withRoom`: with room for a passenger, `still`: not moving up or down. */
    std::optional<int> copterOnWater(bool withRoom, bool still) const;

private:
    int players_;
    const data::LevelDefinition* definition_ = nullptr;
    std::array<Copter, 2> copters_;
    std::vector<Pad> pads_;
    Water water_;
    Rain rain_;
    Energy energy_;
    Fade fade_;
    int passengersLeft_ = 0;
    bool done_ = false;
};

}  // namespace ugh::world
