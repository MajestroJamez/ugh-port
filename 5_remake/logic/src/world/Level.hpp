// The world of the level being played.
#pragma once

#include <optional>
#include <span>
#include <vector>

#include "data/GameData.hpp"
#include "data/levels/LevelDefinition.hpp"
#include "events/Diagnostics.hpp"
#include "events/EventListener.hpp"
#include "world/Delivery.hpp"
#include "world/Energy.hpp"
#include "world/Fade.hpp"
#include "world/copter/Copter.hpp"
#include "world/copter/Copters.hpp"
#include "world/scenery/Pad.hpp"
#include "world/scenery/Rain.hpp"
#include "world/scenery/Water.hpp"
#include "world/session/RandomNumbers.hpp"

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
    void startAttempt(const data::levels::LevelDefinition& definition, const data::SpriteIds& sprites,
                      session::RandomNumbers& random, events::Diagnostics& diagnostics);

    const data::levels::LevelDefinition* definition() const { return definition_; }
    data::levels::Wind wind() const { return definition_ ? definition_->wind : data::levels::Wind::None; }
    bool windy() const { return wind() != data::levels::Wind::None; }

    copter::Copters& copters() { return copters_; }
    const copter::Copters& copters() const { return copters_; }
    int padCount() const { return static_cast<int>(pads_.size()); }
    /** Pad `index` of the level's list (the data names pads so). */
    scenery::Pad& pad(int index) { return pads_[index]; }
    const scenery::Pad& pad(int index) const { return pads_[index]; }
    std::span<scenery::Pad> pads() { return pads_; }
    std::span<const scenery::Pad> pads() const { return pads_; }
    scenery::Water& water() { return water_; }
    const scenery::Water& water() const { return water_; }
    scenery::Rain& rain() { return rain_; }
    const scenery::Rain& rain() const { return rain_; }
    Energy& energy() { return energy_; }
    const Energy& energy() const { return energy_; }
    Fade& fade() { return fade_; }
    const Fade& fade() const { return fade_; }

    /** How many passengers are left, and whether the level is done. */
    const Delivery& delivery() const { return delivery_; }
    /** A passenger finished its route; the last one ends the level (the count never goes below zero). */
    void passengerFinished(events::EventListener& events);

    /** The attempt ends: the level fades out, unless it does already; true when the fade-out starts now. */
    bool fadeOut();
    /** `copter` crashed: the attempt ends - reported unless the level fades out already. */
    void crash(const copter::Copter& copter, events::EventListener& events);

    /** The background is solid at the pixel. */
    bool solid(int x, int y) const { return definition_ && definition_->mask.solid(x, y); }

private:
    const data::levels::LevelDefinition* definition_ = nullptr;
    copter::Copters copters_;
    std::vector<scenery::Pad> pads_;
    scenery::Water water_;
    scenery::Rain rain_;
    Energy energy_;
    Fade fade_;
    Delivery delivery_;
};

}  // namespace ugh::world
