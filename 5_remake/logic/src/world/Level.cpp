#include "world/Level.hpp"

namespace ugh::world {

void Level::startAttempt(const data::levels::LevelDefinition& definition, const data::SpriteIds& sprites,
                         session::RandomNumbers& random, events::Diagnostics& diagnostics) {
    definition_ = &definition;
    energy_.fill();
    fade_.startFadeIn();
    delivery_.start(definition.toDeliver);
    pads_.clear();
    for (const data::levels::PadDefinition& place : definition.pads) pads_.emplace_back(padCount(), place);
    copters_.placeAtStart(definition, sprites);
    water_.fill(definition.water);
    if (windy()) rain_.start(water_.row(), definition.wind, random, diagnostics);
}

/** The fade-out of the last passenger starts even when the level fades out already (one fade step more). */
void Level::passengerFinished(events::EventListener& events) {
    if (!delivery_.finishOne()) return;
    fade_.startFadeOut();
    events.onEvent({events::EventKind::LevelDone});
}

bool Level::fadeOut() {
    if (fade_.fadingOut()) return false;
    fade_.startFadeOut();
    return true;
}

void Level::crash(const copter::Copter& copter, events::EventListener& events) {
    if (!fadeOut()) return;
    events.onEvent({events::EventKind::CopterCrashed, copter.player()});
}

}  // namespace ugh::world
