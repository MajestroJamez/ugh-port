// Passengers (level list B, 113b:1486 .. 2276; Passengers.kt).
//
// Three state machines share the state handler slots of the descriptors (data.cpp checks them):
//   walking   out of the door of the pickup pad, wait, call a copter that lands there, ride, pay, walk to the
//             door of the target pad, next stop of the route;
//   swimming  a walking passenger that fell into the water or was knocked in by a copter: splash, swim, call a
//             copter on the water or sink;
//   standing  the passenger that waits on its pad until a copter picks it up and drops it wherever its pilot wants.
// A state is what a passenger is in from one frame to the next. The functions between them (appear, startCalling,
// board ...) are handlers of the original that lead on to the next state in the same frame.
//
// The sound effects of the original (ADLX 4274, 4632) are events.
#include "passengers.hpp"

#include <algorithm>

#include "bonuses.hpp"
#include "game.hpp"

namespace ugh {

namespace {

constexpr Sprite BUBBLE_DESTINATION = 0x10c;   // + the target pad, at most BUBBLE_DESTINATION_LAST
constexpr Sprite BUBBLE_DESTINATION_LAST = 0x111;
constexpr Sprite BUBBLE_IMPATIENT = 0x112;
constexpr Sprite STANDING_SPRITE = 0x220, DROPPED_SPRITE = 0x221;
constexpr int16_t CALL_TIME = 0x8c;            // frames of calling and of impatient waving
constexpr int16_t QUICK_DELIVERY_TIME = 0xc8;  // frames of riding that still earn a bonus item
constexpr int16_t GRABBED_TARGET = 7;          // the number shown for a standing passenger on board
constexpr int16_t MAX_SPEED = 0x1800;

}  // namespace

extern const PassengerState Arriving, Appearing, Waiting, Calling, Impatient, Boarding, Riding, WalkingAway, Entering,
    Gone, Standing, Hanging, Falling, Splash, Sinking, Swimming, SwimCalling, SwimWaving, SwimBoarding;

/** One passenger's turn in the frame: the passenger, the game, and what the states share. */
class PassengerTurn {
public:
    PassengerTurn(Game& game, int index) : game(game), world(game.world), p(game.world.passengers[index]), index(index) {}

    Game& game;
    World& world;
    Passenger& p;
    const int index;

    const PassengerKind& kind() const { return *p.kind; }

    /** The state from the next frame on. */
    void next(const PassengerState& state) { p.state = &state; }

    /** On in another state in this frame. */
    void jump(const PassengerState& state) {
        p.state = &state;
        state.update(*this);
    }

    Pad& pad(int16_t i) {
        if (i >= 0 && i < static_cast<int>(world.pads.size())) return world.pads[i];
        game.problems.push_back("a passenger refers to pad " + std::to_string(i));
        static Pad nowhere;
        return nowhere;
    }

    /** The copter carrying the passenger (riding, hanging). */
    Copter& carrier() {
        int c = p.carrier();
        if (c == 0 || c == 1) return world.copters[c];
        game.problems.push_back("a passenger carried by copter " + std::to_string(c));
        return world.copters[0];
    }

    /** DEC: true when the counter reached zero. */
    static bool countDown(int16_t& counter) {
        counter = static_cast<int16_t>(counter - 1);
        return counter == 0;
    }

    void restartAnimation() {
        p.anim = -1;
        p.animDelay = 1;
    }

    /** The animation delay; true when it ran out and the next frame is due. */
    bool animationStep() {
        if (!countDown(p.animDelay)) return false;
        p.animDelay = kind().animDelay;
        p.anim++;
        return true;
    }

    /** Shows the current frame of an animation, from its start again at the end. */
    void show(const Animation& animation) {
        while (animation.frame(p.anim) == LIST_END) p.anim = 0;
        p.sprite = animation.frame(p.anim);
    }

    /** A copter landed on the pad with room for a passenger. */
    bool emptyCopterOnPad(int16_t pad) const {
        for (int c = 0; c < world.copterCount(); c++)
            if (world.copters[c].landedPad == pad && world.copters[c].carrying == 0) return true;
        return false;
    }

    /** A copter floating on the water (its bottom at the surface); with `empty` one with room for a passenger. */
    int copterOnWater(bool empty, bool still) const {
        for (int c = 0; c < world.copterCount(); c++) {
            const Copter& copter = world.copters[c];
            if ((still && copter.vy != 0) || (empty && copter.carrying != 0)) continue;
            if (static_cast<int16_t>(copter.pixelY - world.water.row + 0x12) == 0) return c;
        }
        return -1;
    }

    /** The passenger's feet reached the water. */
    bool fellIntoWater();

    /** Knocked over by a copter flying through it (113b:2276 finds a copter in the air touching it). */
    bool knockedIntoWater() {
        int c = touchingCopter(world, kind().box, p.x, p.y);
        if (c < 0 || world.copters[c].landedPad != -1) return false;
        intoWater();
        return true;
    }

    void intoWater();
    void walkToCopter(int copter);

    /** Keeps a swimmer on the water (when the surface moved away from its top). */
    void floatOnSurface() {
        if (world.water.row != p.pixelY) p.y = Fixed::fromPixels(world.water.row - kind().box.y);
    }
};

namespace {

// ---------------------------------------------------------------- the walking passenger

/** 113b:153b - out of the door of the pickup pad; the pad has a passenger waiting now. */
void appear(PassengerTurn& t) {
    Passenger& p = t.p;
    t.next(Appearing);
    Pad& pad = t.pad(p.pickupPad);
    pad.waiting = static_cast<int16_t>(t.index);
    p.pixelY = static_cast<int16_t>(pad.y - t.kind().box.y);
    p.y = Fixed::fromPixels(p.pixelY);
    p.pixelX = static_cast<int16_t>(pad.doorX - t.kind().box.x);
    p.x = Fixed::fromPixels(p.pixelX);
    t.restartAnimation();
}

/** 113b:15b4 */
void startWaiting(PassengerTurn& t) {
    t.next(Waiting);
    t.restartAnimation();
    t.p.counter = -1;
    t.p.bubble = NO_SPRITE;
}

/** 113b:16f6 - shows the bubble with the destination; calls a copter on the pad (or on the water). */
void startCalling(PassengerTurn& t) {
    t.next(t.kind().set == PassengerSet::Swimming ? SwimCalling : Calling);
    t.restartAnimation();
    auto bubble = static_cast<uint16_t>(t.p.targetPad + BUBBLE_DESTINATION);
    t.p.bubble = std::min<uint16_t>(bubble, BUBBLE_DESTINATION_LAST);
    t.p.counter = CALL_TIME;
}

/** 113b:17e6 - the copter left without the passenger. */
void startImpatient(PassengerTurn& t) {
    t.next(t.kind().set == PassengerSet::Swimming ? SwimWaving : Impatient);
    t.restartAnimation();
    t.p.bubble = BUBBLE_IMPATIENT;
    t.p.counter = CALL_TIME;
}

/** 113b:18c8 */
void startBoarding(PassengerTurn& t) {
    t.next(t.kind().set == PassengerSet::Swimming ? SwimBoarding : Boarding);
    t.restartAnimation();
    t.p.bubble = NO_SPRITE;
}

/**
 * 113b:19fb / 19e0 - boards the copter: its fare from the descriptor, the target pad shown. A swimmer gets its
 * walking kind back first (19e0), with that kind's look.
 */
void board(PassengerTurn& t, int copter) {
    Passenger& p = t.p;
    Copter& c = t.world.copters[copter];
    c.fare = t.kind().fare;
    c.fareMin = t.kind().fareMin;
    if (t.kind().set == PassengerSet::Swimming) p.kind = t.kind().other;
    t.next(Riding);
    c.carrying = t.kind().look;
    c.targetPad = t.pad(p.targetPad).number;
    p.setCarrier(copter);
    p.sprite = NO_SPRITE;
    p.bonusTimer = QUICK_DELIVERY_TIME;
    t.pad(p.pickupPad).waiting = -1;
    t.game.report({EventKind::PassengerBoarded, copter, t.index});
}

/** 113b:1a7e - delivered: pays the fare times the multiplier; a quick delivery drops a bonus item. */
void pay(PassengerTurn& t) {
    Passenger& p = t.p;
    World& world = t.world;
    t.next(WalkingAway);
    int copter = p.carrier();
    Copter& c = t.carrier();
    c.carrying = 0;
    c.targetPad = 0;
    p.x = c.x + Fixed::fromPixels(0x10 - t.kind().box.x);
    // the original multiplies the multiplier as a word, with the zero byte after it
    uint32_t points = static_cast<uint32_t>(static_cast<uint16_t>(c.fare)) * world.multiplier;
    t.game.addScore(points);
    t.game.report({EventKind::PassengerPaid, copter, t.index, static_cast<int>(points)});
    if (p.bonusTimer != 0 && world.multiplier < t.game.data.multiplierLimit(world.difficulty)) {
        dropBonus(t.game, t.game.data.quickDeliveryBonus(), c.x + Fixed::fromPixels(16), c.y + Fixed::fromPixels(10), 0, 0);
        t.game.report({EventKind::QuickDelivery, copter, t.index});
    }
    p.y = Fixed::fromPixels(t.pad(p.targetPad).y - t.kind().box.y);
    t.restartAnimation();
}

/** 113b:1bbe */
void startEntering(PassengerTurn& t) {
    t.next(Entering);
    t.restartAnimation();
}

void gone(PassengerTurn& t) {
    t.p.sprite = NO_SPRITE;
    t.next(Gone);
}

/** 113b:149c - the next stop of the route, or the passenger leaves the level when the route is done. */
void nextStop(PassengerTurn& t) {
    Passenger& p = t.p;
    World& world = t.world;
    if (p.route.finished()) {
        auto left = static_cast<uint8_t>(world.passengersLeft - 1);
        if ((left & 0x80) == 0) {
            world.passengersLeft = left;
            if (left == 0) {   // the last one: the level is done
                world.fade.startFadeOut();
                world.levelDone = true;
                t.game.report({EventKind::LevelDone});
            }
        }
        p.sprite = NO_SPRITE;
        t.jump(Gone);
        return;
    }
    p.targetPad = p.route.to();
    p.pickupPad = p.route.from();
    p.timer = threeHalves(p.route.delay());
    t.next(Arriving);
}

/** 113b:1509 - hidden, waits for its time and for the pickup pad to be free. */
void arriving(PassengerTurn& t) {
    Passenger& p = t.p;
    p.sprite = NO_SPRITE;
    if (p.timer != 0 && !PassengerTurn::countDown(p.timer)) return;
    if (t.pad(p.pickupPad).waiting != -1) return;
    appear(t);
}

/** 113b:1582 - the door animation, then waiting. */
void appearing(PassengerTurn& t) {
    if (!t.animationStep()) return;
    Sprite frame = t.kind().appearing->frame(t.p.anim);
    if (frame == LIST_END) { startWaiting(t); return; }
    t.p.sprite = frame;
}

/** 113b:15d7 - walks to the waiting spot of the pad and waits there for a copter. */
void waiting(PassengerTurn& t) {
    Passenger& p = t.p;
    if (t.fellIntoWater()) return;
    if (t.animationStep()) {
        int16_t waitX = t.pad(p.pickupPad).waitX;
        auto spot = static_cast<int16_t>(p.pixelX + t.kind().box.x);
        if (spot == waitX) {
            if (p.counter != 1) p.anim = 0;   // just arrived
            t.show(*t.kind().standing);
            p.counter = 1;
        } else {
            p.counter = 0;
            if (spot < waitX) { t.show(*t.kind().walking.right); p.x += Fixed::fromPixels(1); }
            else { t.show(*t.kind().walking.left); p.x -= Fixed::fromPixels(1); }
        }
    }
    if (t.knockedIntoWater()) return;
    if (t.emptyCopterOnPad(p.pickupPad)) startCalling(t);
}

/** 113b:172a - a copter is on the pad: the passenger calls it for a while, then walks to it. */
void calling(PassengerTurn& t) {
    if (t.fellIntoWater()) return;
    if (t.knockedIntoWater()) return;
    int c = t.world.copterOnPad(t.p.pickupPad);
    if (c < 0 || t.world.copters[c].carrying != 0) { startImpatient(t); return; }
    if (t.animationStep()) t.show(*t.kind().waving);
    if (PassengerTurn::countDown(t.p.counter)) startBoarding(t);
}

/** 113b:180a - waves impatiently for a while, then calls the next copter or waits again. */
void impatient(PassengerTurn& t) {
    if (t.fellIntoWater()) return;
    if (t.knockedIntoWater()) return;
    if (t.animationStep()) t.show(*t.kind().waving);
    if (!PassengerTurn::countDown(t.p.counter)) return;
    if (t.emptyCopterOnPad(t.p.pickupPad)) startCalling(t); else startWaiting(t);
}

/** 113b:18e6 - walks to the landed copter. */
void boarding(PassengerTurn& t) {
    if (t.fellIntoWater()) return;
    if (t.knockedIntoWater()) return;
    int c = t.world.copterOnPad(t.p.pickupPad);
    if (c < 0) { startImpatient(t); return; }
    t.walkToCopter(c);
}

/** 113b:1a42 - riding: the fare drops to its minimum, the bonus time runs out, until the copter lands at the target. */
void riding(PassengerTurn& t) {
    Passenger& p = t.p;
    Copter& c = t.carrier();
    if (static_cast<uint16_t>(c.fare) > static_cast<uint16_t>(c.fareMin)) c.fare = static_cast<int16_t>(c.fare - 1);
    if (p.bonusTimer > 0) p.bonusTimer = static_cast<int16_t>(p.bonusTimer - 1);
    if (p.targetPad == c.landedPad) pay(t);
}

/** 113b:1b29 - walks from the copter to the door of the target pad. */
void walkingAway(PassengerTurn& t) {
    Passenger& p = t.p;
    if (!t.animationStep()) return;
    int16_t door = t.pad(p.targetPad).doorX;
    auto spot = static_cast<int16_t>(p.pixelX + t.kind().box.x);
    if (spot == door) { startEntering(t); return; }
    if (spot < door) { t.show(*t.kind().walking.right); p.x += Fixed::fromPixels(1); }
    else { t.show(*t.kind().walking.left); p.x -= Fixed::fromPixels(1); }
    if (static_cast<int16_t>(p.x.pixels() + t.kind().box.x) == door) startEntering(t);
}

/** 113b:1bd6 - the door animation, then the next stop of the route. */
void entering(PassengerTurn& t) {
    Passenger& p = t.p;
    if (!t.animationStep()) return;
    p.sprite = t.kind().entering->frame(p.anim);
    if (t.kind().entering->frame(p.anim + 1) != LIST_END) return;
    p.route.stop++;
    t.jump(NextStop);
}

void nothing(PassengerTurn&) {}

// ---------------------------------------------------------------- the standing passenger

/** 113b:1c48 - picked up by a copter without a passenger. */
void grab(PassengerTurn& t, int copter) {
    Copter& c = t.world.copters[copter];
    t.next(Hanging);
    c.carrying = t.kind().look;
    c.targetPad = GRABBED_TARGET;
    t.p.setCarrier(copter);
    t.p.sprite = NO_SPRITE;
    t.game.report({EventKind::PassengerBoarded, copter, t.index});
}

/** 113b:1c81 - let go: falls with the copter's speed, from under the copter. */
void drop(PassengerTurn& t) {
    Passenger& p = t.p;
    int copter = p.carrier();
    Copter& c = t.carrier();
    c.carrying = 0;
    c.targetPad = 0;
    p.timer = static_cast<int16_t>(c.vx >> 5);
    p.vy = static_cast<int16_t>(c.vy >> 5);
    p.x = c.x + Fixed::fromPixels(16) - Fixed::fromPixels(t.kind().box.x);
    p.y = c.y + Fixed::fromPixels(10) - Fixed(t.kind().box.y * 16);   // half its height
    p.sprite = DROPPED_SPRITE;
    t.next(Falling);
    t.game.report({EventKind::PassengerDropped, copter, t.index});
}

/** 113b:1c0f */
void startStanding(PassengerTurn& t) {
    t.next(Standing);
    t.restartAnimation();
}

/** 113b:1c27 - waits for a copter without a passenger to touch it. */
void standing(PassengerTurn& t) {
    int c = touchingCopter(t.world, t.kind().box, t.p.x, t.p.y);
    if (c >= 0 && t.world.copters[c].carrying == 0) { grab(t, c); return; }
    t.p.sprite = STANDING_SPRITE;
}

/** 113b:1c6b - hangs on the copter until its pilot presses fire. */
void hanging(PassengerTurn& t) {
    if (t.carrier().keys.fire) drop(t);
}

/** 113b:1cee - falls; lands on a pad or leaves the screen. */
void falling(PassengerTurn& t) {
    Passenger& p = t.p;
    const Box& box = t.kind().box;
    Fixed x = p.x + Fixed(p.timer);
    if (x <= Fixed(-0x200) || x >= Fixed(0x2800)) { gone(t); return; }
    p.x = x;
    p.vy = static_cast<int16_t>(p.vy + 2);
    if (p.vy < 0) { p.y += Fixed(p.vy); return; }
    Fixed before = p.y;
    Fixed y = before + Fixed(p.vy);
    if (y >= Fixed(0x1800)) { gone(t); return; }
    p.y = y;
    auto feet = static_cast<int16_t>(y.pixels() + box.y);
    auto feetBefore = static_cast<int16_t>(before.pixels() + box.y);
    auto middle = static_cast<int16_t>(p.x.pixels() + box.x);
    for (int i = 0; i < t.world.padCount; i++) {
        const Pad& pad = t.world.pads[i];
        if (feetBefore <= pad.y && feet >= pad.y && middle >= pad.left && middle <= pad.right) {
            p.y = Fixed::fromPixels(pad.y - box.y);
            t.jump(StartStanding);
            return;
        }
    }
}

// ---------------------------------------------------------------- the swimming passenger

/** 113b:1da8 - into the water: the pickup pad is free again. */
void startSplash(PassengerTurn& t) {
    t.next(Splash);
    t.pad(t.p.pickupPad).waiting = -1;
    t.p.bubble = NO_SPRITE;
    t.restartAnimation();
    t.p.vy = 0;
    t.game.report({EventKind::PassengerInWater, -1, t.index});
}

/** 113b:1e9c */
void startSinking(PassengerTurn& t) {
    t.next(Sinking);
    t.p.bubble = NO_SPRITE;
    t.restartAnimation();
    t.p.vy = 0;
}

/** 113b:1f24 */
void startSwimming(PassengerTurn& t) {
    t.next(Swimming);
    t.restartAnimation();
    t.p.timer = t.kind().swimTime;
}

/** 113b:1dd5 - falls into the water, goes under and floats up to the surface. */
void splash(PassengerTurn& t) {
    Passenger& p = t.p;
    const Box& box = t.kind().box;
    t.animationStep();
    t.show(*t.kind().standing);
    auto depth = static_cast<int16_t>(p.pixelY - box.y - t.world.water.row);
    int vy = p.vy;
    if (depth < 0) vy += 0x27;           // above the water: falls
    else if (vy > 0) vy -= 0x175;        // in it: buoyancy
    else vy -= 0x5c;
    p.vy = std::clamp<int16_t>(static_cast<int16_t>(vy), -MAX_SPEED, MAX_SPEED);
    p.y += perFrame(p.vy);
    if (p.vy >= 0) return;
    if (static_cast<int16_t>(p.y.pixels() - box.y) > t.world.water.row) return;
    p.y = Fixed::fromPixels(t.world.water.row - box.y);
    startSwimming(t);
}

/** 113b:1ec0 - drowns: sinks until it is off the bottom. */
void sinking(PassengerTurn& t) {
    Passenger& p = t.p;
    t.animationStep();
    t.show(*t.kind().standing);
    p.vy = static_cast<int16_t>(p.vy + 0x27);
    Fixed y = p.y + perFrame(p.vy);
    if (static_cast<uint16_t>(y.raw()) < 0x1800) { p.y = y; return; }
    t.next(Gone);
    p.sprite = NO_SPRITE;
}

/** 113b:1f43 - swims; a copter still on the water nearby rescues it, else it drowns when its time is up. */
void swimming(PassengerTurn& t) {
    t.animationStep();
    t.show(*t.kind().waving);
    t.floatOnSurface();
    if (t.kind().rescuable && t.copterOnWater(true, true) >= 0) { startCalling(t); return; }
    if (PassengerTurn::countDown(t.p.timer)) startSinking(t);
}

/** 113b:1fe2 - a copter is on the water: waves to it, or swims again when it left. */
void swimCalling(PassengerTurn& t) {
    t.floatOnSurface();
    if (t.copterOnWater(false, false) < 0) { startImpatient(t); return; }
    if (t.animationStep()) t.show(*t.kind().waving);
    if (PassengerTurn::countDown(t.p.counter)) startBoarding(t);
}

/** 113b:2068 - waves for a while, then sinks. */
void swimWaving(PassengerTurn& t) {
    t.floatOnSurface();
    if (t.animationStep()) t.show(*t.kind().waving);
    if (PassengerTurn::countDown(t.p.counter)) startSinking(t);
}

/** 113b:20c1 - swims to the copter on the water. */
void swimBoarding(PassengerTurn& t) {
    t.floatOnSurface();
    int c = t.copterOnWater(true, false);
    if (c >= 0) t.walkToCopter(c); else startImpatient(t);
}

}  // namespace

bool PassengerTurn::fellIntoWater() {
    if (static_cast<int16_t>((kind().box.y >> 1) + p.pixelY - world.water.row) < 0) return false;
    p.y = Fixed::fromPixels(world.water.row + kind().box.y);
    pad(p.pickupPad).waiting = -1;
    p.bubble = NO_SPRITE;
    intoWater();
    return true;
}

void PassengerTurn::intoWater() {
    p.kind = kind().other;
    startSplash(*this);
}

/** Walks (or swims) towards a copter and boards it when level with it. */
void PassengerTurn::walkToCopter(int copter) {
    bool step = animationStep();
    auto spot = static_cast<int16_t>(p.pixelX + kind().box.x - 0x10);
    int16_t copterX = world.copters[copter].pixelX;
    if (spot > copterX) {
        show(*kind().walking.left);
        if (step) p.x -= Fixed::fromPixels(1);
    } else if (spot < copterX) {
        show(*kind().walking.right);
        if (step) p.x += Fixed::fromPixels(1);
    } else {
        board(*this, copter);
    }
}

const PassengerState NextStop{"NextStop", nextStop};
const PassengerState Arriving{"Arriving", arriving};
const PassengerState Appearing{"Appearing", appearing};
const PassengerState Waiting{"Waiting", waiting};
const PassengerState Calling{"Calling", calling};
const PassengerState Impatient{"Impatient", impatient};
const PassengerState Boarding{"Boarding", boarding};
const PassengerState Riding{"Riding", riding};
const PassengerState WalkingAway{"WalkingAway", walkingAway};
const PassengerState Entering{"Entering", entering};
const PassengerState Gone{"Gone", nothing};
const PassengerState StartStanding{"StartStanding", startStanding};
const PassengerState Standing{"Standing", standing};
const PassengerState Hanging{"Hanging", hanging};
const PassengerState Falling{"Falling", falling};
const PassengerState Splash{"Splash", splash};
const PassengerState Sinking{"Sinking", sinking};
const PassengerState Swimming{"Swimming", swimming};
const PassengerState SwimCalling{"SwimCalling", swimCalling};
const PassengerState SwimWaving{"SwimWaving", swimWaving};
const PassengerState SwimBoarding{"SwimBoarding", swimBoarding};

const std::vector<const PassengerState*>& passengerStates() {
    static const std::vector<const PassengerState*> all = {
        &NextStop, &Arriving, &Appearing, &Waiting, &Calling, &Impatient, &Boarding, &Riding, &WalkingAway, &Entering,
        &Gone, &StartStanding, &Standing, &Hanging, &Falling, &Splash, &Sinking, &Swimming, &SwimCalling, &SwimWaving,
        &SwimBoarding};
    return all;
}

bool fallingDown(const Passenger& passenger) {
    return passenger.kind->set == PassengerSet::Standing && passenger.state == &Falling && passenger.vy >= 0;
}

/** 113b:1486 - Passengers.kt passengersUpdate: every passenger's state. */
void updatePassengers(Game& game) {
    for (int i = 0; i < game.world.passengerCount; i++) {
        PassengerTurn turn(game, i);
        if (turn.p.state && turn.p.kind) turn.p.state->update(turn);
        else game.problems.push_back("passenger " + std::to_string(i) + " without a state or kind");
    }
}

/** Frame.kt frameAfterKeys (the drawing of the passengers): the pixel positions the states read next frame. */
void updatePassengerPixels(World& world) {
    for (int i = 0; i < world.passengerCount; i++) {
        Passenger& p = world.passengers[i];
        if (p.sprite == NO_SPRITE) continue;
        p.pixelX = p.x.pixels();
        p.pixelY = p.y.pixels();
    }
}

}  // namespace ugh
