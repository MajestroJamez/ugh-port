// A level attempt (113b:3d66 and 3976, Level.kt levelSetup and loadLevel): the state of a new attempt and the load
// of the level: copters, water, pads (list A), passengers (list B), enemies (list C), rain.
#include <algorithm>
#include <string>

#include "enemies.hpp"
#include "game.hpp"
#include "passengers.hpp"

namespace ugh {

namespace {

constexpr int16_t FULL_ENERGY = 0x5a3b;

/** 113b:3976 - Level.kt loadLevel. */
void loadLevel(Game& game) {
    World& w = game.world;
    w.level = game.data.level(w.players, w.levelNumber);
    if (!w.level) {
        game.problems.push_back("no level " + std::to_string(w.levelNumber) + " for " + std::to_string(w.players) + " players");
        return;
    }
    const LevelDefinition& level = *w.level;
    w.passengersLeft = level.toDeliver;
    w.wind = level.wind;

    for (int p = 0; p < 2; p++) {   // both copters, also with one player
        Copter& c = w.copters[p];
        c.x = level.startX[p];
        c.pixelX = c.x.pixels();
        c.y = level.startY[p];
        c.pixelY = c.y.pixels();
        c.landedPad = -1;
        c.rotorCounter = -1;
        c.rotor = game.data.rotorFirst(p);
        c.carrying = 0;
        c.targetPad = 0;
        c.fare = 0;
        c.vx = 0;
        c.vy = 0;
    }
    w.water.level = level.water;
    w.water.row = level.water.pixels();

    w.padCount = static_cast<int>(level.pads.size());
    std::copy(level.pads.begin(), level.pads.end(), w.pads.begin());

    w.passengerCount = static_cast<int>(level.passengers.size());
    for (int i = 0; i < w.passengerCount; i++) {
        const PassengerPlacement& placement = level.passengers[i];
        Passenger& p = w.passengers[i];
        p.kind = placement.kind;
        p.startPad = placement.pad;
        p.route = placement.route;
        p.sprite = NO_SPRITE;
        p.bubble = NO_SPRITE;
        if (placement.kind->set == PassengerSet::Standing) {
            p.state = &StartStanding;
            p.x = placement.x;
            p.y = placement.y;
        } else {
            p.state = &NextStop;
        }
    }

    w.enemyCount = static_cast<int>(level.enemies.size());
    for (int i = 0; i < w.enemyCount; i++) {
        const EnemyPlacement& placement = level.enemies[i];
        Enemy& e = w.enemies[i];
        e.kind = placement.kind;
        placement.kind->behavior->place(e, placement);
    }

    startRain(game);
}

}  // namespace

/** 113b:3d66 - Level.kt levelSetup up to the caption: a new attempt at the level, and its load. */
void startLevelAttempt(Game& game) {
    World& w = game.world;
    // the original clears DGROUP:2648 .. 27cf: the held keys, the water's counters, the fade, the level-done flag
    // (and what the drawing keeps)
    for (Copter& c : w.copters) c.keys = {};
    w.water.hold = 0;
    w.water.toggle = 0;
    w.water.surfaceFrame = 0;
    w.water.surfaceDelay = 0;
    w.levelDone = false;
    w.energy = FULL_ENERGY;
    w.fade = {0, 2};
    loadLevel(game);
}

}  // namespace ugh
