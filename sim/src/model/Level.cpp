#include "model/Level.hpp"

#include <string>

namespace ugh::model {

namespace {

using core::Fixed;

// a falling passenger hits an enemy with this point (from its top left corner) inside the enemy's box (from its top
// left corner, in pixels)
constexpr core::Word HIT_POINT_X = 12, HIT_POINT_Y = 8, ENEMY_WIDTH = 38, ENEMY_HEIGHT = 28;

}  // namespace

Level::Level(const data::GameData& data, GameSession& session, core::EventListener& events, core::Diagnostics& diagnostics)
    : data_(data), session_(session), events_(events), diagnostics_(diagnostics) {
    reset();
}

void Level::reset() {
    s_ = Snapshot{};
    for (Copter& c : copters_) c = Copter();
    for (Pad& p : pads_) p = Pad();
    for (int i = 0; i < PASSENGERS; i++) passengers_[i] = Passenger(i);
    for (int i = 0; i < ENEMIES; i++) enemies_[i] = Enemy(i);
    bonuses_ = BonusSlots();
    water_ = Water();
    rain_ = Rain();
}

void Level::startAttempt() {
    // the original clears DGROUP:2648 .. 27cf: the held keys, the water's counters, the fade, the level-done flag
    // (and what the drawing keeps)
    for (Copter& c : copters_) c.controls() = Controls{};
    water_.startAttempt();
    s_.done = false;
    s_.energy.fill();
    s_.fade.startFadeIn();
}

void Level::loadLists(const data::LevelDefinition& definition) {
    s_.passengersLeft = definition.toDeliver;
    s_.wind = definition.wind;
    s_.padCount = static_cast<int>(definition.pads.size());
    for (int i = 0; i < s_.padCount; i++) pads_[i] = Pad(definition.pads[i]);
    s_.passengerCount = static_cast<int>(definition.passengers.size());
    s_.enemyCount = static_cast<int>(definition.enemies.size());
}

void Level::passengerFinished() {
    auto left = static_cast<uint8_t>(s_.passengersLeft - 1);
    if (left & 0x80) return;   // the original does not count below zero
    s_.passengersLeft = left;
    if (left != 0) return;
    // the last one: the level is done
    s_.fade.startFadeOut();
    s_.done = true;
    report({core::EventKind::LevelDone});
}

void Level::hideSprites() {
    for (Enemy& e : enemies_) e.hide();
    for (Passenger& p : passengers_) p.hide();
    bonuses_.clear();
}

void Level::updatePassengers() {
    for (int i = 0; i < s_.passengerCount; i++) {
        Passenger& p = passengers_[i];
        if (p.snapshot().state && p.snapshot().kind) p.update(*this);
        else diagnostics_.report("passenger " + std::to_string(i) + " without a state or kind");
    }
}

void Level::updateEnemies() {
    for (int i = 0; i < s_.enemyCount; i++) {
        Enemy& e = enemies_[i];
        if (e.snapshot().state && e.snapshot().kind) e.update(*this);
        else diagnostics_.report("enemy " + std::to_string(i) + " without a state or kind");
    }
}

void Level::updatePassengerPixels() {
    for (int i = 0; i < s_.passengerCount; i++) passengers_[i].updatePixels();
}

core::Word Level::waterSpeed() const {
    const data::LevelDefinition* level = definition();
    return level ? level->waterSpeed : core::Word(0);
}

bool Level::solid(int index) const {
    const data::LevelDefinition* level = definition();
    return level && level->mask.solid(index);
}

int Level::copterLandedOn(int pad) const {
    for (int c = 0; c < copterCount(); c++)
        if (copters_[c].landedOn(pad)) return c;
    return NONE;
}

bool Level::emptyCopterLandedOn(int pad) const {
    for (int c = 0; c < copterCount(); c++)
        if (copters_[c].landedOn(pad) && copters_[c].hasRoom()) return true;
    return false;
}

int Level::copterOnWater(bool withRoom, bool still) const {
    for (int c = 0; c < copterCount(); c++) {
        const Copter& copter = copters_[c];
        if ((still && !copter.stillVertically()) || (withRoom && !copter.hasRoom())) continue;
        if (copter.depthIn(water_.row()) == 0) return c;
    }
    return NONE;
}

int Level::fallingPassengerNear(const Enemy& enemy) const {
    for (int i = 0; i < s_.passengerCount; i++) {
        const Passenger& p = passengers_[i];
        if (!p.fallingDown()) continue;
        Fixed hitY = p.y() + Fixed::fromPixels(HIT_POINT_Y), hitX = p.x() + Fixed::fromPixels(HIT_POINT_X);
        if (hitY >= enemy.y() && hitY - Fixed::fromPixels(ENEMY_HEIGHT) <= enemy.y() && hitX >= enemy.x() &&
            hitX - Fixed::fromPixels(ENEMY_WIDTH) <= enemy.x())
            return i;
    }
    return NONE;
}

}  // namespace ugh::model
