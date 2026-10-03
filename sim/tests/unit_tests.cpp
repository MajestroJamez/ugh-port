// Unit tests of the core's value types, services and state machines, next to the golden replays (which check
// everything against the original). Usage: ugh_sim_tests <ugh-sim.bin>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "bonuses/Falling.hpp"
#include "bonuses/Lying.hpp"
#include "core/AmigaScale.hpp"
#include "core/Countdown.hpp"
#include "core/EventQueue.hpp"
#include "core/Fixed.hpp"
#include "core/Random.hpp"
#include "core/Speed.hpp"
#include "core/Word.hpp"
#include "data/CollisionMask.hpp"
#include "data/GameData.hpp"
#include "data/GameDataLoader.hpp"
#include "game/Game.hpp"
#include "game/LevelLoader.hpp"
#include "model/Water.hpp"
#include "passengers/PassengerState.hpp"
#include "physics/CopterPhysics.hpp"
#include "ugh_sim.h"

namespace {

using namespace ugh;
using core::Fixed;
using core::Word;

int failures = 0;

#define CHECK(condition)                                                                   \
    do {                                                                                   \
        if (!(condition)) {                                                                \
            std::printf("  %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition);    \
            failures++;                                                                    \
        }                                                                                  \
    } while (false)

const data::GameData* gameData = nullptr;

/** A game at the start of a level attempt: one player, level `number` loaded, the copters in the air. */
struct Setup {
    game::Game game{*gameData};
    core::EventQueue events;
    model::Level& level = game.level();

    explicit Setup(int number = 0) {
        game.addListener(events);
        model::GameSession::Snapshot session = level.session().snapshot();
        session.levelNumber = number;
        level.session().restore(session);
        game.newGame();
        game::LevelLoader::startAttempt(level);
    }

    std::string passengerState(int i) const { return level.passenger(i).state().name(); }

    /** Runs the passengers' state machines until passenger i is in the state; false after `frames` frames. */
    bool runPassengersUntil(int i, const std::string& state, int frames = 2000) {
        for (int f = 0; f < frames; f++) {
            if (passengerState(i) == state) return true;
            level.updatePassengers();
            level.updatePassengerPixels();
        }
        return passengerState(i) == state;
    }

    /** Lands copter c on a pad, at x. */
    void land(int c, int pad, Word x) {
        model::Copter& copter = level.copter(c);
        copter.moveToX(Fixed::fromPixels(x));
        copter.land(pad);
    }

    /** Puts copter c over the middle of pad 1, `height` px above it. */
    void hover(int c, int height) {
        model::Copter& copter = level.copter(c);
        const model::Pad& pad = level.pad(1);
        copter.moveToX(Fixed::fromPixels(Word((pad.left().value() + pad.right().value()) / 2) - 0x10));
        copter.moveToY(Fixed::fromPixels(pad.y() - 0x14 - height));
    }

    bool reported(core::EventKind kind) const {
        for (const core::Event& e : events.events())
            if (e.kind == kind) return true;
        return false;
    }
};

// ---------------------------------------------------------------- core

void words() {
    CHECK(Word(0x7fff) + 1 == Word(-0x8000));            // wraps like a register
    CHECK(Word(0x12345) == Word(0x2345));                // an int is cut to 16 bits
    CHECK(Word(-1).bits() == 0xffff && Word(-1).value() == -1);
    CHECK((Word(-5) >> 1) == Word(-3));                  // SAR rounds down
    CHECK((Word(0x4001) << 1) == Word(-0x7ffe));         // SHL wraps
    CHECK(Word(-1) < Word(1));                           // signed (JL)
    CHECK(Word::unsignedLess(Word(1), Word(-1)));        // unsigned (JB)
    Word w = 0;
    CHECK(--w == Word(-1));
}

void fixedPoint() {
    CHECK(Fixed(0x7fff) + Fixed(1) == Fixed(-0x8000));   // wraps like a register
    CHECK(Fixed(-1).pixels() == -1);                     // SAR rounds down
    CHECK(Fixed(63).pixels() == 1);
    CHECK(Fixed(-33).wholePixel() == Fixed(-64));
    CHECK(Fixed::fromPixels(0x400) == Fixed(-0x8000));
    CHECK(core::amigaRowsToPc(-5) == -3);                // -5 - (-5 >> 2) = -5 + 2
    CHECK(core::amigaFramesToPc(7) == 10);
}

void speed() {
    using core::Speed;
    CHECK(Speed(-0x41).perFrame() == Fixed(-2));         // SAR 6 rounds down
    CHECK(Speed(0x40).perFrame() == Fixed(1));
    CHECK(Speed(0x2000).clamped(Speed(0x1800)) == Speed(0x1800));
    CHECK(Speed(-0x2000).clamped(Speed(0x1800)) == Speed(-0x1800));
    CHECK((Speed(-0x3f) >> 1) == Speed(-0x20));
}

void countdown() {
    core::Countdown c;
    c.start(2);
    CHECK(!c.tick());
    CHECK(c.tick());                                     // done on the second tick
    CHECK(c.remaining() == 0);
    CHECK(!c.tick() && c.remaining() == -1);             // from zero it runs through the whole word
}

void random() {
    core::Random r;
    CHECK(r.next(0x140) == 3);   // worked by hand: the words add up 0x140, 0x140, 0x140, then 0x280
    CHECK((r.snapshot() == core::Random::Snapshot{0x280, 0x140, 0x140, 0x140}));
    CHECK(r.next(0x140) == 14);
    r.restore({0xffff, 0xffff, 0, 0});   // the carries run through the chain
    CHECK(r.next(1) == 0);
    CHECK((r.snapshot() == core::Random::Snapshot{1, 0, 1, 1}));
    for (int i = 0; i < 1000; i++) CHECK(r.next(7) < 7);
}

// ---------------------------------------------------------------- data

void collisionMask() {
    std::vector<uint8_t> bits(data::CollisionMask::WIDTH / 8 * data::CollisionMask::HEIGHT);
    bits[4 * 48 + 47] = 0x01;   // pixel 383 of row 4
    data::CollisionMask mask(bits);
    CHECK(mask.solid(4 * 384 + 383));
    CHECK(mask.solid(5 * 384 - 1));      // a probe running over the right edge reads the next row
    CHECK(!mask.solid(4 * 384 + 382));
    CHECK(!mask.solid(-1));              // outside the page nothing is solid
    CHECK(!mask.solid(384 * 192));
}

void dataLoaded() {
    using Type = data::PassengerKind::Type;
    CHECK(gameData->levelCount(1) == 69 && gameData->levelCount(2) == 81);
    CHECK(gameData->level(1, 69) == nullptr);
    const data::LevelDefinition& level = *gameData->level(1, 0);
    CHECK(level.pads.size() == 3 && level.passengers.size() == 3 && level.enemies.size() == 1);
    CHECK(level.startX[0] == Fixed(4608) && level.startY[0] == Fixed(2080));
    const data::PassengerKind& walking = *gameData->passengerKind(0x7720);
    CHECK(walking.type == Type::Walking && walking.other->type == Type::Swimming);
    CHECK(walking.other->other == &walking);
    CHECK(!gameData->passengerKind(0x77fe)->rescuable && walking.other->rescuable);
    CHECK(gameData->quickDeliveryBonus().effect == data::BonusKind::Effect::Multiplier);
    CHECK(level.enemies[0].kind->type == data::EnemyKind::Type::Tree && level.enemies[0].drops != nullptr);
    CHECK(gameData->rotorEnd(0) == gameData->rotorFirst(1));
}

// ---------------------------------------------------------------- model, physics, input

void keyboard() {
    Setup s;
    auto& p0 = s.level.copter(0).controls();
    auto& p1 = s.level.copter(1).controls();
    s.game.key(0xe0);
    s.game.key(0x48);   // cursor up: player 0
    CHECK(p0.up && !p1.up);
    s.game.key(0x11);   // W: player 1
    CHECK(p1.up);
    s.game.key(0xe0);
    s.game.key(0xc8);   // cursor up released
    CHECK(!p0.up && p1.up);
    s.game.key(0xe0);
    s.game.key(0x2a);   // the fake shift of an extended key: nothing
    s.game.key(0x48);   // keypad 8 without the prefix: player 1 up
    CHECK(!p0.up);
    s.game.key(0xe0);
    s.game.key(0x1f);   // E0 1F is no key: the sequence starts again
    s.game.key(0x1f);   // S: player 1 right
    CHECK(p1.right && !p0.right);
}

void waterMovesEverySecondFrame() {
    model::Water w;
    w.fillTo(Fixed(0x100));
    w.move(-0x40);   // toggle 1: no movement
    CHECK(w.level() == Fixed(0x100));
    w.move(-0x40);   // toggle 0: moves by the speed
    CHECK(w.level() == Fixed(0xc0));
    CHECK(w.snapshot().hold == 0xff);   // the row changed: the next frame holds
    w.move(-0x40);
    CHECK(w.level() == Fixed(0xc0) && w.snapshot().hold == 0);
}

void copterFallsOntoPad() {
    Setup s;
    model::Copter& c = s.level.copter(0);
    s.hover(0, 8);
    physics::CopterPhysics physics(s.level);
    for (int f = 0; f < 200 && !c.landed(); f++) physics.fly(0);
    CHECK(c.landedOn(1));
    CHECK(c.speedX() == core::Speed(0) && c.speedY() == core::Speed(0));
    CHECK(!s.level.fade().fadingOut());   // a soft touch-down
}

void hardImpactCrashes() {
    Setup s;
    model::Copter& c = s.level.copter(0);
    s.hover(0, 1);
    c.setSpeed(core::Speed(0), core::Speed(0x1800));
    physics::CopterPhysics(s.level).fly(0);
    CHECK(s.level.fade().fadingOut());
    CHECK(s.reported(core::EventKind::CopterCrashed));
}

// ---------------------------------------------------------------- state machines

void passengerRidesAndPays() {
    Setup s;
    CHECK(s.passengerState(0) == "NextStop");
    CHECK(s.runPassengersUntil(0, "Waiting"));
    model::Passenger& p = s.level.passenger(0);
    int pickup = p.pickupPad(), target = p.targetPad();
    CHECK(!s.level.pad(pickup).free());
    // a copter lands next to the passenger: it calls, then walks to it and boards
    s.land(0, pickup, s.level.pad(pickup).waitX() - 0x20);
    CHECK(s.runPassengersUntil(0, "Calling", 600));
    CHECK(p.snapshot().bubble != data::NO_SPRITE);
    CHECK(s.runPassengersUntil(0, "Riding", 2000));
    const model::Copter::Snapshot& copter = s.level.copter(0).snapshot();
    CHECK(copter.carrying == p.kind().look);
    CHECK(copter.targetPad == s.level.pad(target).number());
    CHECK(s.level.pad(pickup).free());
    CHECK(s.reported(core::EventKind::PassengerBoarded));
    // at the target it pays the fare (one less after this frame, times the multiplier 1)
    Word fare = s.level.copter(0).fare();
    s.land(0, target, s.level.copter(0).pixelX());
    s.level.updatePassengers();
    CHECK(s.passengerState(0) == "WalkingAway");
    CHECK(s.level.session().snapshot().score == static_cast<uint32_t>(fare.value() - 1));
    CHECK(s.level.copter(0).hasRoom());
    CHECK(s.reported(core::EventKind::PassengerPaid));
    // a quick delivery drops a bonus item for the multiplier
    const model::BonusItem& bonus = s.level.bonuses()[11];
    CHECK(bonus.inUse() && &bonus.kind() == &gameData->quickDeliveryBonus());
}

void passengerKnockedIntoWater() {
    Setup s;
    CHECK(s.runPassengersUntil(0, "Waiting"));
    model::Passenger& p = s.level.passenger(0);
    // a copter flies right through it
    model::Copter& c = s.level.copter(0);
    c.takeOff();
    c.moveToX(p.x() - Fixed::fromPixels(8));
    c.moveToY(p.y() - Fixed::fromPixels(8));
    s.level.updatePassengers();
    CHECK(s.passengerState(0) == "Splash");
    CHECK(p.kind().type == data::PassengerKind::Type::Swimming);
    CHECK(s.reported(core::EventKind::PassengerInWater));
}

void bonusCollectedOnce() {
    Setup s;
    s.level.energy() = model::Energy(0x5a00);
    const data::BonusKind* energy = nullptr;
    for (const data::BonusKind* k : gameData->level(1, 0)->enemies[0].drops->items)   // the tree's
        if (k->effect == data::BonusKind::Effect::Energy) { energy = k; break; }
    CHECK(energy != nullptr);
    if (!energy) return;
    model::Copter& c = s.level.copter(0);
    bonuses::Falling::drop(s.level, *energy, c.x(), c.y(), Fixed(0), 0);
    model::BonusItem& b = s.level.bonuses()[11];
    CHECK(b.inUse());
    // lying at the copter
    b.changeState(bonuses::Lying::instance, s.level);
    b.moveToX(c.x() + Fixed::fromPixels(8));
    b.moveToY(c.y());
    s.level.bonuses().update(s.level);
    CHECK(!b.inUse());
    CHECK(s.level.energy().value() == model::Energy::FULL);   // filled up to the maximum, not over it
    CHECK(s.reported(core::EventKind::BonusCollected));
}

// ---------------------------------------------------------------- the game

void wholeGameEndsWithEsc() {
    Setup s;
    s.game.reset();
    int frames = 0, result = UGH_SIM_CONTINUE;
    for (; frames < 5000 && result == UGH_SIM_CONTINUE; frames++) {
        if (frames == 300) s.game.key(0x39);   // space: past the caption
        if (frames == 500) s.game.key(0x01);   // Esc: gives up
        result = s.game.step();
    }
    CHECK(result == UGH_SIM_GAME_OVER);
    CHECK(s.reported(core::EventKind::LevelCaption));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: ugh_sim_tests <ugh-sim.bin>\n");
        return 2;
    }
    std::string error;
    auto loaded = data::GameDataLoader::load(argv[1], error);
    if (!loaded) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }
    gameData = loaded.get();
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"words", words},
        {"fixed point", fixedPoint},
        {"speed", speed},
        {"countdown", countdown},
        {"random numbers", random},
        {"collision mask", collisionMask},
        {"game data", dataLoaded},
        {"keyboard", keyboard},
        {"water", waterMovesEverySecondFrame},
        {"copter lands on a pad", copterFallsOntoPad},
        {"hard impact crashes", hardImpactCrashes},
        {"passenger rides and pays", passengerRidesAndPays},
        {"passenger knocked into the water", passengerKnockedIntoWater},
        {"bonus item collected", bonusCollectedOnce},
        {"whole game, given up", wholeGameEndsWithEsc},
    };
    for (const auto& [name, test] : tests) {
        int before = failures;
        test();
        std::printf("%s %s\n", failures == before ? "ok  " : "FAIL", name);
    }
    std::printf("%d failures\n", failures);
    return failures == 0 ? 0 : 1;
}
