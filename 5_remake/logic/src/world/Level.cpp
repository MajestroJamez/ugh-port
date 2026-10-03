#include "world/Level.hpp"

namespace ugh::world {

void Level::startAttempt(const data::LevelDefinition& definition, const data::SpriteIds& sprites, RandomNumbers& random,
                         events::Diagnostics& diagnostics) {
    definition_ = &definition;
    for (Copter& copter : copters_) copter.releaseKeys();
    done_ = false;
    energy_.fill();
    fade_.startFadeIn();
    passengersLeft_ = definition.toDeliver;
    pads_.clear();
    for (const data::PadDefinition& place : definition.pads) pads_.emplace_back(place);
    // both copters, also in the one-player mode
    for (int player = 0; player < 2; player++)
        copters_[player].placeAtStart(definition.startX[player], definition.startY[player], sprites.firstRotor[player]);
    water_.fill(definition.water);
    if (windy()) rain_.start(water_.row(), definition.wind, random, diagnostics);
}

void Level::passengerFinished(events::EventListener& events) {
    if (passengersLeft_ == 0) return;
    if (--passengersLeft_ != 0) return;
    fade_.startFadeOut();
    done_ = true;
    events.onEvent({events::EventKind::LevelDone});
}

std::optional<int> Level::copterLandedOn(int pad) const {
    for (int c = 0; c < copterCount(); c++)
        if (copters_[c].landedOn(pad)) return c;
    return std::nullopt;
}

bool Level::emptyCopterLandedOn(int pad) const {
    for (int c = 0; c < copterCount(); c++)
        if (copters_[c].landedOn(pad) && copters_[c].hasRoom()) return true;
    return false;
}

std::optional<int> Level::firstCopterOnWater(bool withRoom, bool still) const {
    for (int c = 0; c < copterCount(); c++) {
        const Copter& copter = copters_[c];
        if ((still && !copter.stillVertically()) || (withRoom && !copter.hasRoom())) continue;
        if (copter.depthIn(water_.row()) == 0) return c;
    }
    return std::nullopt;
}

}  // namespace ugh::world
