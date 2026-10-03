// Unit tests of the core's services and state machines, next to the golden replays (which check everything
// against the original). Usage: ugh_sim_tests <ugh-sim.bin>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "bonuses.hpp"
#include "data.hpp"
#include "game.hpp"
#include "keyboard.hpp"
#include "passengers.hpp"
#include "random.hpp"

namespace {

using namespace ugh;

int failures = 0;

#define CHECK(condition)                                                                   \
    do {                                                                                   \
        if (!(condition)) {                                                                \
            std::printf("  %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition);    \
            failures++;                                                                    \
        }                                                                                  \
    } while (false)

const GameData* data = nullptr;

/** A game at the start of a level attempt: one player, level `number` loaded, the copters in the air. */
struct Setup {
    Game game{*data};
    World& w = game.world;

    explicit Setup(int number = 0) {
        w.players = 1;
        w.difficulty = 1;
        w.levelNumber = static_cast<uint16_t>(number);
        game.newGame();
        startLevelAttempt(game);
    }

    const char* passengerState(int i) const { return w.passengers[i].state->name; }

    /** Runs the passengers' state machines until passenger i is in the state; false after `frames` frames. */
    bool runPassengersUntil(int i, const char* state, int frames = 2000) {
        for (int f = 0; f < frames; f++) {
            if (std::string(passengerState(i)) == state) return true;
            updatePassengers(game);
            updatePassengerPixels(w);
        }
        return std::string(passengerState(i)) == state;
    }

    /** Lands copter c on a pad, at x. */
    void land(int c, int pad, int16_t x) {
        Copter& copter = w.copters[c];
        copter.landedPad = static_cast<int16_t>(pad);
        copter.pixelX = x;
        copter.x = Fixed::fromPixels(x);
        copter.vx = copter.vy = 0;
    }

    bool reported(EventKind kind) const {
        for (const Event& e : game.events)
            if (e.kind == kind) return true;
        return false;
    }
};

void fixedPoint() {
    CHECK(Fixed(0x7fff) + Fixed(1) == Fixed(-0x8000));   // wraps like a register
    CHECK(Fixed(-1).pixels() == -1);                     // SAR rounds down
    CHECK(Fixed(63).pixels() == 1);
    CHECK(Fixed(-33).wholePixel() == Fixed(-64));
    CHECK(Fixed::fromPixels(0x400) == Fixed(-0x8000));
    CHECK(perFrame(-0x41) == Fixed(-2));
    CHECK(threeQuarters(-5) == -3);                      // -5 - (-5 >> 2) = -5 + 2
    CHECK(threeHalves(7) == 10);
}

void random() {
    Random r;
    CHECK(r.next(0x140) == 3);   // worked by hand: the words add up 0x140, 0x140, 0x140, then 0x280
    CHECK((r.state == std::array<uint16_t, 4>{0x280, 0x140, 0x140, 0x140}));
    CHECK(r.next(0x140) == 14);
    r.state = {0xffff, 0xffff, 0, 0};   // the carries run through the chain
    CHECK(r.next(1) == 0);
    CHECK((r.state == std::array<uint16_t, 4>{1, 0, 1, 1}));
    for (int i = 0; i < 1000; i++) CHECK(r.next(7) < 7);
}

void collisionMask() {
    std::vector<uint8_t> bits(CollisionMask::WIDTH / 8 * CollisionMask::HEIGHT);
    bits[4 * 48 + 47] = 0x01;   // pixel 383 of row 4
    CollisionMask mask(bits);
    CHECK(mask.solid(4 * 384 + 383));
    CHECK(mask.solid(5 * 384 - 1));      // a probe running over the right edge reads the next row
    CHECK(!mask.solid(4 * 384 + 382));
    CHECK(!mask.solid(-1));              // outside the page nothing is solid
    CHECK(!mask.solid(384 * 192));
}

void keyboard() {
    std::array<Copter, 2> copters;
    Keyboard k(data->keys());
    k.deliver(0xe0, copters);
    k.deliver(0x48, copters);   // cursor up: player 0
    CHECK(copters[0].keys.up && !copters[1].keys.up);
    k.deliver(0x11, copters);   // W: player 1
    CHECK(copters[1].keys.up);
    k.deliver(0xe0, copters);
    k.deliver(0xc8, copters);   // cursor up released
    CHECK(!copters[0].keys.up && copters[1].keys.up);
    k.deliver(0xe0, copters);
    k.deliver(0x2a, copters);   // the fake shift of an extended key: nothing
    k.deliver(0x48, copters);   // keypad 8 without the prefix: player 1 up
    CHECK(!copters[0].keys.up);
    k.deliver(0xe0, copters);
    k.deliver(0x1f, copters);   // E0 1F is no key: the sequence starts again
    k.deliver(0x1f, copters);   // S: player 1 right
    CHECK(copters[1].keys.right && !copters[0].keys.right);
    Keyboard::Reading r = k.read();
    CHECK(r.scancode == 0x1f && r.changed);
    CHECK(!k.read().changed);
}

void gameData() {
    CHECK(data->levelCount(1) == 69 && data->levelCount(2) == 81);
    CHECK(data->level(1, 69) == nullptr);
    const LevelDefinition& level = *data->level(1, 0);
    CHECK(level.pads.size() == 3 && level.passengers.size() == 3 && level.enemies.size() == 1);
    CHECK(level.startX[0] == Fixed(4608) && level.startY[0] == Fixed(2080));
    const PassengerKind& walking = *data->passengerKind(0x7720);
    CHECK(walking.set == PassengerSet::Walking && walking.other->set == PassengerSet::Swimming);
    CHECK(walking.other->other == &walking);
    CHECK(!data->passengerKind(0x77fe)->rescuable && walking.other->rescuable);
    CHECK(data->quickDeliveryBonus().effect == BonusKind::Effect::Multiplier);
    CHECK(std::string(level.enemies[0].kind->name) == "tree" && level.enemies[0].drops != nullptr);
}

void waterMovesEverySecondFrame() {
    World w;
    w.water.level = Fixed(0x100);
    w.water.row = w.water.level.pixels();
    moveWater(w);   // toggle 1: no movement
    CHECK(w.water.level == Fixed(0x100));
    moveWater(w);   // toggle 0: moves by the level's speed (none without a level)
    CHECK(w.water.toggle == 0 && w.water.hold == 0);
}

void copterFallsOntoPad() {
    Setup s;
    Copter& c = s.w.copters[0];
    const Pad& pad = s.w.pads[1];
    // over the middle of pad 1, a little above it
    c.pixelX = static_cast<int16_t>((pad.left + pad.right) / 2 - 0x10);
    c.x = Fixed::fromPixels(c.pixelX);
    c.pixelY = static_cast<int16_t>(pad.y - 0x14 - 8);
    c.y = Fixed::fromPixels(c.pixelY);
    for (int f = 0; f < 200 && !c.landed(); f++) flyCopter(s.game, 0);
    CHECK(c.landedPad == 1);
    CHECK(c.vx == 0 && c.vy == 0);
    CHECK(!s.w.fade.fadingOut());   // a soft touch-down
}

void hardImpactCrashes() {
    Setup s;
    Copter& c = s.w.copters[0];
    const Pad& pad = s.w.pads[1];
    c.pixelX = static_cast<int16_t>((pad.left + pad.right) / 2 - 0x10);
    c.x = Fixed::fromPixels(c.pixelX);
    c.pixelY = static_cast<int16_t>(pad.y - 0x14 - 1);
    c.y = Fixed::fromPixels(c.pixelY);
    c.vy = 0x1800;
    flyCopter(s.game, 0);
    CHECK(s.w.fade.fadingOut());
    CHECK(s.reported(EventKind::CopterCrashed));
}

void passengerRidesAndPays() {
    Setup s;
    CHECK(std::string(s.passengerState(0)) == "NextStop");
    CHECK(s.runPassengersUntil(0, "Waiting"));
    Passenger& p = s.w.passengers[0];
    int16_t pickup = p.pickupPad, target = p.targetPad;
    CHECK(s.w.pads[pickup].waiting == 0);
    // a copter lands next to the passenger: it calls, then walks to it and boards
    s.land(0, pickup, static_cast<int16_t>(s.w.pads[pickup].waitX - 0x20));
    CHECK(s.runPassengersUntil(0, "Calling", 600));
    CHECK(p.bubble != NO_SPRITE);
    CHECK(s.runPassengersUntil(0, "Riding", 2000));
    CHECK(s.w.copters[0].carrying == p.kind->look);
    CHECK(s.w.copters[0].targetPad == s.w.pads[target].number);
    CHECK(s.w.pads[pickup].waiting == -1);
    CHECK(s.reported(EventKind::PassengerBoarded));
    // at the target it pays the fare (one less after this frame, times the multiplier 1)
    int16_t fare = s.w.copters[0].fare;
    s.land(0, target, s.w.copters[0].pixelX);
    updatePassengers(s.game);
    CHECK(std::string(s.passengerState(0)) == "WalkingAway");
    CHECK(s.w.score == static_cast<uint32_t>(fare - 1));
    CHECK(s.w.copters[0].carrying == 0);
    CHECK(s.reported(EventKind::PassengerPaid));
    // a quick delivery drops a bonus item for the multiplier
    CHECK(s.w.bonuses[11].used() && s.w.bonuses[11].kind == &data->quickDeliveryBonus());
}

void passengerKnockedIntoWater() {
    Setup s;
    CHECK(s.runPassengersUntil(0, "Waiting"));
    Passenger& p = s.w.passengers[0];
    // a copter flies right through it
    Copter& c = s.w.copters[0];
    c.landedPad = -1;
    c.x = p.x - Fixed::fromPixels(8);
    c.y = p.y - Fixed::fromPixels(8);
    updatePassengers(s.game);
    CHECK(std::string(s.passengerState(0)) == "Splash");
    CHECK(p.kind->set == PassengerSet::Swimming);
    CHECK(s.reported(EventKind::PassengerInWater));
}

void bonusCollectedOnce() {
    Setup s;
    s.w.energy = 0x5a00;
    const BonusKind* energy = nullptr;
    for (const BonusKind* k : data->level(1, 0)->enemies[0].drops->items)   // the tree's
        if (k->effect == BonusKind::Effect::Energy) { energy = k; break; }
    CHECK(energy != nullptr);
    if (!energy) return;
    Copter& c = s.w.copters[0];
    dropBonus(s.game, *energy, c.x, c.y, 0, 0);
    BonusItem& b = s.w.bonuses[11];
    CHECK(b.used());
    // lying at the copter
    for (const BonusState* state : bonusStates())
        if (std::string(state->name) == "Lying") b.state = state;
    b.x = c.x + Fixed::fromPixels(8);
    b.y = c.y;
    b.vx = 100;
    updateBonuses(s.game);
    CHECK(!b.used());
    CHECK(s.w.energy == 0x5a3b);   // filled up to the maximum, not over it
    CHECK(s.reported(EventKind::BonusCollected));
}

void wholeGameEndsWithEsc() {
    Setup s;
    s.game.reset();
    s.w.players = 1;
    s.w.difficulty = 1;
    int frames = 0, result = UGH_SIM_CONTINUE;
    for (; frames < 5000 && result == UGH_SIM_CONTINUE; frames++) {
        if (frames == 300) s.game.key(0x39);   // space: past the caption
        if (frames == 500) s.game.key(0x01);   // Esc: gives up
        result = s.game.step();
    }
    CHECK(result == UGH_SIM_GAME_OVER);
    CHECK(s.reported(EventKind::LevelCaption));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: ugh_sim_tests <ugh-sim.bin>\n");
        return 2;
    }
    std::string error;
    auto loaded = GameData::load(argv[1], error);
    if (!loaded) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }
    data = loaded.get();
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"fixed point", fixedPoint},
        {"random numbers", random},
        {"collision mask", collisionMask},
        {"keyboard", keyboard},
        {"game data", gameData},
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
