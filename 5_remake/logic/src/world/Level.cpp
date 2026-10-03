#include "world/Level.hpp"

namespace ugh::world {

void Level::startAttempt(const data::LevelDefinition& definition, const data::SpriteIds& sprites, RandomNumbers& random,
                         events::Diagnostics& diagnostics) {
    definition_ = &definition;
    energy_.fill();
    fade_.startFadeIn();
    delivery_.start(definition.toDeliver);
    pads_.clear();
    for (const data::PadDefinition& place : definition.pads) pads_.emplace_back(place);
    copters_.placeAtStart(definition, sprites);
    water_.fill(definition.water);
    if (windy()) rain_.start(water_.row(), definition.wind, random, diagnostics);
}

void Level::passengerFinished(events::EventListener& events) {
    if (!delivery_.finishOne()) return;
    fade_.startFadeOut();
    events.onEvent({events::EventKind::LevelDone});
}

}  // namespace ugh::world
