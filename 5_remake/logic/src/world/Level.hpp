// The world of the level being played.
#pragma once

#include <optional>
#include <vector>

#include "data/GameData.hpp"
#include "data/LevelDefinition.hpp"
#include "events/Diagnostics.hpp"
#include "events/EventListener.hpp"
#include "world/Copters.hpp"
#include "world/Delivery.hpp"
#include "world/Energy.hpp"
#include "world/Fade.hpp"
#include "world/Pad.hpp"
#include "world/Rain.hpp"
#include "world/RandomNumbers.hpp"
#include "world/Water.hpp"

namespace ugh::world {

/**
 * The world of the level being played: the copters (and what the entities ask about them), the pads, the water, the
 * rain, the energy, the fade and how many passengers are left. It lasts for the whole game: an attempt reloads it from
 * the level's definition, but the copters keep their effort and the rain keeps its floor row.
 */
class Level {
public:
    explicit Level(int players = 1) : copters_(players) {}

    /** A new attempt at `definition`: the copters at their start, the pads, the water, full energy, fading in. */
    void startAttempt(const data::LevelDefinition& definition, const data::SpriteIds& sprites, RandomNumbers& random,
                      events::Diagnostics& diagnostics);

    const data::LevelDefinition* definition() const { return definition_; }
    data::Wind wind() const { return definition_ ? definition_->wind : data::Wind::None; }
    bool windy() const { return wind() != data::Wind::None; }

    Copters& copters() { return copters_; }
    const Copters& copters() const { return copters_; }
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

    /** How many passengers are left, and whether the level is done. */
    const Delivery& delivery() const { return delivery_; }
    /** A passenger finished its route; the last one ends the level (the count never goes below zero). */
    void passengerFinished(events::EventListener& events);

    /** The background is solid at the pixel. */
    bool solid(int x, int y) const { return definition_ && definition_->mask.solid(x, y); }

private:
    const data::LevelDefinition* definition_ = nullptr;
    Copters copters_;
    std::vector<Pad> pads_;
    Water water_;
    Rain rain_;
    Energy energy_;
    Fade fade_;
    Delivery delivery_;
};

}  // namespace ugh::world
