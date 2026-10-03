// Enemies (level list C, at most 4) - 113b:2363 .. 113b:2b7e, 2196 and 22f1, Objects.kt.
//
//   flyer   waits, screeches and flies across the screen at the height of a copter; touching its target ends the life
//   walker  walks along its pad; when a copter lands there it turns to it and charges, throwing the copter up
//   blower  blows copters in front of it sideways
//   tree    sways; a passenger dropped onto it bounces off and shakes a bonus item out of it
// A standing passenger dropped onto a flyer, walker or blower stuns it and scores. As with the passengers, a state
// is what an enemy is in from frame to frame and the functions between the states lead on to the next one.
//
// The sound effects of the original (ADLX 425b, 4260, 4265, 4274) are events; the flap sound's handle (2d61)
// is not kept.
#include "enemies.hpp"

#include "bonuses.hpp"
#include "game.hpp"
#include "passengers.hpp"

namespace ugh {

namespace {

constexpr Sprite HIT_PASSENGER_SPRITE = 0x222;
constexpr Sprite TREE_HIT_SPRITE = 0xe8;
constexpr int16_t SCREECH_TIME = 0x46;
constexpr int16_t WATCH_TIME = 0x8c;
constexpr int16_t STUNNED_TIME = 0x15e;
constexpr int16_t TREE_REST_TIME = 0xd2;
constexpr int16_t BLOW = 0x29;   // push of the blower per frame

}  // namespace

extern const EnemyState FlyerInit, FlyerWait, FlyerWait2, Flying, FlyerFalling, WalkerInit, Walking, Watching,
    Charging, Recovering, Stunned, BlowerInit, Blowing, BlowerWait, TreeInit, TreeSwaying, TreeWait, Inactive;

/** One enemy's turn in the frame: the enemy, the game, and what the states share. */
class EnemyTurn {
public:
    EnemyTurn(Game& game, int index) : game(game), world(game.world), e(game.world.enemies[index]), index(index) {}

    Game& game;
    World& world;
    Enemy& e;
    const int index;

    const EnemyKind& kind() const { return *e.kind; }

    /** The state from the next frame on. */
    void next(const EnemyState& state) { e.state = &state; }

    /** On in another state in this frame. */
    void jump(const EnemyState& state) {
        e.state = &state;
        state.update(*this);
    }

    static bool countDown(int16_t& counter) {
        counter = static_cast<int16_t>(counter - 1);
        return counter == 0;
    }

    void restartAnimation() {
        e.anim = -1;
        e.animDelay = 1;
    }

    /** The animation delay; true when it ran out: the next frame is due and the delay starts again at `delay`. */
    bool animationStep(int16_t delay) {
        if (!countDown(e.animDelay)) return false;
        e.animDelay = delay;
        e.anim++;
        return true;
    }

    /** Shows the current frame of an animation, from its start again at the end. */
    void show(const Animation& animation) {
        while (animation.frame(e.anim) == LIST_END) e.anim = 0;
        e.sprite = animation.frame(e.anim);
    }

    const Pad& pad() {
        if (e.pad >= 0 && e.pad < static_cast<int>(world.pads.size())) return world.pads[e.pad];
        game.problems.push_back("an enemy refers to pad " + std::to_string(e.pad));
        static const Pad nowhere;
        return nowhere;
    }

    /** The first copter landed on the enemy's pad; -1 = none. */
    int copterOnPad() const { return world.copterOnPad(e.pad); }

    /** Turns to the copter and runs towards it. */
    void faceCopter(int copter) {
        Fixed speed = e.vx;
        if (e.x < world.copters[copter].x) {
            e.facing = 2;
            if (speed < Fixed(0)) speed = -speed;
        } else {
            e.facing = 0;
            if (speed >= Fixed(0)) speed = -speed;
        }
        e.vx = speed;
    }

    /** 113b:2196 - a standing passenger falling from a copter close to the enemy; -1 = none. */
    int fallingPassengerNear() const {
        for (int i = 0; i < world.passengerCount; i++) {
            const Passenger& p = world.passengers[i];
            if (!fallingDown(p)) continue;
            Fixed bottom = p.y + Fixed::fromPixels(8), left = p.x + Fixed::fromPixels(12);
            if (bottom >= e.y && bottom - Fixed::fromPixels(28) <= e.y && left >= e.x && left - Fixed::fromPixels(38) <= e.x)
                return i;
        }
        return -1;
    }

    /** A falling passenger bounces off the enemy (and shows it was hit). */
    bool passengerBounces(bool showHit = true) {
        int i = fallingPassengerNear();
        if (i < 0) return false;
        Passenger& p = world.passengers[i];
        p.vy = static_cast<int16_t>(-p.vy);
        if (showHit) p.sprite = HIT_PASSENGER_SPRITE;
        return true;
    }

    /** Stunned by a passenger: the score of the kind. */
    void scoreStun() {
        game.addScore(static_cast<uint16_t>(kind().score));
        game.report({EventKind::EnemyStunned, -1, index, kind().score});
    }
};

namespace {

// ---------------------------------------------------------------- flyer (7630)

/** 113b:23b0 */
void flyerScreech(EnemyTurn& t) {
    t.next(FlyerWait2);
    t.e.timer = SCREECH_TIME;
    t.game.report({EventKind::FlyerScreech, -1, t.index});
}

/** 113b:23ea - takes the other player as its target (in the team mode) and comes in from the far side at its height. */
void flyerStart(EnemyTurn& t) {
    Enemy& e = t.e;
    int target = e.facing ^ 1;
    if (target >= static_cast<int16_t>(t.world.players)) target = 0;
    e.facing = static_cast<int16_t>(target);
    if (target != 0 && target != 1) {
        t.game.problems.push_back("a flyer hunts player " + std::to_string(target));
        target = 0;
    }
    const Copter& c = t.world.copters[target];
    if (c.x < Fixed(0x1400)) {   // the copter is on the left: in from the right edge
        if (e.vx >= Fixed(0)) e.vx = -e.vx;
        e.x = Fixed(0x27e0);
        e.table = t.kind().moving.left;
    } else {
        if (e.vx < Fixed(0)) e.vx = -e.vx;
        e.x = Fixed(-0x3e0);
        e.table = t.kind().moving.right;
    }
    Fixed y = c.y + Fixed(0x340);
    if (y > t.world.water.level) y = t.world.water.level;
    y -= Fixed(0x340);
    if (y < Fixed(-0x80)) y = Fixed(-0x80);
    e.y = y;
    t.next(Flying);
    t.game.report({EventKind::FlyerFlapStart, -1, t.index});
}

/** 113b:252b - a passenger fell on it: it drops out of the sky. */
void flyerHit(EnemyTurn& t) {
    Enemy& e = t.e;
    t.next(FlyerFalling);
    t.scoreStun();
    e.timer = 0;
    e.sprite = (e.vx < Fixed(0) ? t.kind().stunned.left : t.kind().stunned.right)->frame(0);
}

/** 113b:2379 */
void flyerInit(EnemyTurn& t) {
    t.next(FlyerWait);
    t.restartAnimation();
    t.e.sprite = NO_SPRITE;
    t.e.timer = t.e.startDelay;
}

/** 113b:239f */
void flyerWait(EnemyTurn& t) {
    if (EnemyTurn::countDown(t.e.timer)) flyerScreech(t);
}

/** 113b:23d9 */
void flyerWait2(EnemyTurn& t) {
    if (EnemyTurn::countDown(t.e.timer)) flyerStart(t);
}

/** 113b:2493 - flies across the screen; touching its target's copter ends the life. */
void flying(EnemyTurn& t) {
    Enemy& e = t.e;
    Fixed x = e.x + e.vx;
    if (x <= Fixed(-0x400) || x >= Fixed(0x2800)) {
        t.game.report({EventKind::FlyerFlapStop, -1, t.index});
        t.jump(FlyerInit);
        return;
    }
    e.x = x;
    if (!t.animationStep(4)) return;
    if (auto flight = std::get_if<const Animation*>(&e.table)) t.show(**flight);
    else t.game.problems.push_back("a flyer without its flight animation");
    if (t.passengerBounces()) {
        t.game.report({EventKind::FlyerFlapStop, -1, t.index});
        flyerHit(t);
        return;
    }
    int c = touchingCopter(t.world, t.kind().box, e.x, e.y);
    if (c < 0 || c != e.facing || t.world.fade.fadingOut()) return;
    t.world.fade.startFadeOut();
    t.game.report({EventKind::CopterCrashed, c});
}

/** 113b:255e - falls down, faster and faster, until it is off the screen; then it starts again. */
void flyerFalling(EnemyTurn& t) {
    Enemy& e = t.e;
    Fixed x = e.vx + e.x;
    if (x <= Fixed(-0x400) || x >= Fixed(0x2800)) { t.jump(FlyerInit); return; }
    e.x = x;
    if (e.timer < 0x28) e.timer++;
    Fixed y = Fixed(e.timer) + e.y;
    if (y >= Fixed(0x1800)) { t.jump(FlyerInit); return; }
    e.y = y;
}

// ---------------------------------------------------------------- walker (766c)

/** 113b:2667 - a copter landed on its pad. */
void startWatching(EnemyTurn& t) {
    t.next(Watching);
    t.restartAnimation();
    t.e.timer = WATCH_TIME;
}

/** 113b:272e */
void startCharging(EnemyTurn& t) {
    t.next(Charging);
    t.restartAnimation();
    t.e.timer = 0;
}

/** 113b:2830 / 288f - after charging (the copter left, or it was thrown). */
void startRecovering(EnemyTurn& t) {
    t.next(Recovering);
    t.restartAnimation();
}

/** 113b:28ee - a passenger fell on it. */
void walkerStunned(EnemyTurn& t) {
    t.next(Stunned);
    t.scoreStun();
    t.restartAnimation();
    t.e.timer = STUNNED_TIME;
}

/** 113b:25b1 */
void walkerInit(EnemyTurn& t) {
    t.next(Walking);
    t.restartAnimation();
}

/** 113b:25c9 - walks along its pad, turning at the ends; watches a copter that lands there. */
void walking(EnemyTurn& t) {
    Enemy& e = t.e;
    if (EnemyTurn::countDown(e.animDelay)) {
        const Pad& pad = t.pad();
        e.x += e.vx;
        int16_t x = e.x.pixels();
        if (x < pad.left || static_cast<int16_t>(x + 0x20) >= pad.right) {
            e.vx = -e.vx;
            e.facing ^= 2;
        }
        e.animDelay = 4;
        e.anim++;
        t.show(*t.kind().moving.facing(e.facing));
    }
    if (t.copterOnPad() >= 0) { startWatching(t); return; }
    if (t.passengerBounces()) walkerStunned(t);
}

/** 113b:2681 - looks at the landed copter for a while, then charges. */
void watching(EnemyTurn& t) {
    if (EnemyTurn::countDown(t.e.timer)) { startCharging(t); return; }
    if (t.animationStep(5)) t.show(*t.kind().watching.facing(t.e.facing));
    if (t.passengerBounces()) { walkerStunned(t); return; }
    int c = t.copterOnPad();
    if (c < 0) { t.jump(WalkerInit); return; }
    t.faceCopter(c);
}

/** 113b:2748 - charges at the copter, faster and faster; hitting it throws the copter into the air. */
void charging(EnemyTurn& t) {
    Enemy& e = t.e;
    if (t.animationStep(4)) t.show(*t.kind().charging.facing(e.facing));
    if (t.passengerBounces()) { walkerStunned(t); return; }
    int c = t.copterOnPad();
    if (c < 0) { startRecovering(t); return; }
    t.faceCopter(c);
    e.timer = static_cast<int16_t>(e.facing - 1 + e.timer);   // speeds up towards the copter
    e.x += Fixed(e.timer) + e.vx;
    int hit = touchingCopter(t.world, t.kind().box, e.x, e.y);
    if (hit < 0) return;
    Copter& copter = t.world.copters[hit];
    copter.y -= Fixed::fromPixels(1);
    auto speed = static_cast<int16_t>(e.vx.raw() + e.timer);
    copter.vx = static_cast<int16_t>(speed * 32);
    copter.vy = static_cast<int16_t>(speed * 16);
    copter.landedPad = -1;
    startRecovering(t);
}

/** 113b:2844 - one run of the animation, then walking again. */
void recovering(EnemyTurn& t) {
    if (t.animationStep(4)) {
        Sprite frame = t.kind().recovering.facing(t.e.facing)->frame(t.e.anim);
        if (frame == LIST_END) { t.jump(WalkerInit); return; }
        t.e.sprite = frame;
    }
    if (t.passengerBounces()) walkerStunned(t);
}

/** 113b:2914 */
void stunned(EnemyTurn& t) {
    if (EnemyTurn::countDown(t.e.timer)) { t.jump(WalkerInit); return; }
    if (t.animationStep(4)) t.show(*t.kind().stunned.facing(t.e.facing));
}

// ---------------------------------------------------------------- blower (76a8)

/** 113b:2a53 - a passenger fell on it. */
void blowerStunned(EnemyTurn& t) {
    t.next(BlowerWait);
    t.scoreStun();
    t.e.timer = STUNNED_TIME;
    t.e.sprite = t.kind().stunned.left->frame(0);
}

/** 113b:295b */
void blowerInit(EnemyTurn& t) {
    t.next(Blowing);
    t.restartAnimation();
}

/** 113b:2973 - blows: copters in front of it are pushed sideways, to and fro with the animation. */
void blowing(EnemyTurn& t) {
    Enemy& e = t.e;
    const Box& box = t.kind().box;
    if (t.animationStep(0x0f)) {
        if (e.anim == 3) t.game.report({EventKind::BlowerBlow, -1, t.index});
        t.show(*t.kind().charging.left);
    }
    Fixed bottom = e.y + Fixed::fromPixels(box.y - 9), top = e.y + Fixed::fromPixels(box.y - 30);
    Fixed right = e.x + Fixed::fromPixels(box.x - 16), left = e.x + Fixed::fromPixels(box.x - 104 - 26);
    for (int c = 0; c < t.world.copterCount(); c++) {
        Copter& copter = t.world.copters[c];
        if (bottom >= copter.y && top <= copter.y && right >= copter.x && left <= copter.x) {
            // the first three frames blow one way, the rest the other (the original compares the byte offset with 5)
            copter.vx = static_cast<int16_t>(copter.vx + (e.anim < 3 ? BLOW : -BLOW));
        }
    }
    if (t.passengerBounces(false)) blowerStunned(t);
}

/** 113b:2a76 */
void blowerWait(EnemyTurn& t) {
    if (EnemyTurn::countDown(t.e.timer)) t.jump(BlowerInit);
}

// ---------------------------------------------------------------- tree (76e4)

/** 113b:2b0c - the passenger bounced off: the next bonus item of the tree's list drops where it hit. */
void treeCatch(EnemyTurn& t, const Passenger& p) {
    Enemy& e = t.e;
    t.next(TreeWait);
    e.timer = TREE_REST_TIME;
    e.sprite = TREE_HIT_SPRITE;
    auto drops = std::get_if<DropCursor>(&e.table);
    if (!drops || drops->finished()) {
        t.game.problems.push_back("a tree without bonus items to drop");
        return;
    }
    dropBonus(t.game, drops->next(), p.x, p.y, p.timer, static_cast<int16_t>(static_cast<int16_t>(-p.vy) >> 2));
    drops->index++;
    t.game.report({EventKind::TreeDrop, -1, t.index});
}

/** 113b:2a87 */
void treeInit(EnemyTurn& t) {
    t.next(TreeSwaying);
    t.restartAnimation();
}

/** 113b:2ab5 - sways; a passenger falling on it bounces off (half as high) and shakes a bonus item out. */
void treeSwaying(EnemyTurn& t) {
    if (!t.animationStep(6)) return;
    t.show(*t.kind().moving.left);
    int i = t.fallingPassengerNear();
    if (i < 0) return;
    Passenger& p = t.world.passengers[i];
    p.vy = static_cast<int16_t>(static_cast<int16_t>(-p.vy) >> 1);
    p.sprite = HIT_PASSENGER_SPRITE;
    treeCatch(t, p);
}

/** 113b:2b58 - rests, then sways again, or stays still when its bonus items are all gone. */
void treeWait(EnemyTurn& t) {
    if (!EnemyTurn::countDown(t.e.timer)) return;
    auto drops = std::get_if<DropCursor>(&t.e.table);
    t.next(drops && !drops->finished() ? TreeSwaying : Inactive);
}

void nothing(EnemyTurn&) {}

// ---------------------------------------------------------------- placing them (113b:3b21)

void placeFlyer(Enemy& e, const EnemyPlacement& p) {
    e.startDelay = p.startDelay;
    e.vx = p.vx;
    e.facing = 1;   // the first flight goes for player 0
    e.state = &FlyerInit;
}

void placeWalker(Enemy& e, const EnemyPlacement& p) {
    e.pad = p.pad;
    e.x = p.x;
    e.y = p.y;
    e.vx = p.vx;
    e.facing = 0;
    e.state = &WalkerInit;
}

void placeBlower(Enemy& e, const EnemyPlacement& p) {
    e.x = p.x;
    e.y = p.y;
    e.state = &BlowerInit;
}

void placeTree(Enemy& e, const EnemyPlacement& p) {
    e.pad = p.pad;
    e.x = p.x;
    e.y = p.y;
    e.table = DropCursor{p.drops, 0};
    e.state = &TreeInit;
}

}  // namespace

const EnemyBehavior Flyer{placeFlyer};
const EnemyBehavior Walker{placeWalker};
const EnemyBehavior Blower{placeBlower};
const EnemyBehavior Tree{placeTree};

const EnemyState FlyerInit{"FlyerInit", flyerInit};
const EnemyState FlyerWait{"FlyerWait", flyerWait};
const EnemyState FlyerWait2{"FlyerWait2", flyerWait2};
const EnemyState Flying{"Flying", flying};
const EnemyState FlyerFalling{"FlyerFalling", flyerFalling};
const EnemyState WalkerInit{"WalkerInit", walkerInit};
const EnemyState Walking{"Walking", walking};
const EnemyState Watching{"Watching", watching};
const EnemyState Charging{"Charging", charging};
const EnemyState Recovering{"Recovering", recovering};
const EnemyState Stunned{"Stunned", stunned};
const EnemyState BlowerInit{"BlowerInit", blowerInit};
const EnemyState Blowing{"Blowing", blowing};
const EnemyState BlowerWait{"BlowerWait", blowerWait};
const EnemyState TreeInit{"TreeInit", treeInit};
const EnemyState TreeSwaying{"Tree", treeSwaying};
const EnemyState TreeWait{"TreeWait", treeWait};
const EnemyState Inactive{"Inactive", nothing};

const std::vector<const EnemyState*>& enemyStates() {
    static const std::vector<const EnemyState*> all = {
        &FlyerInit, &FlyerWait, &FlyerWait2, &Flying, &FlyerFalling, &WalkerInit, &Walking, &Watching, &Charging,
        &Recovering, &Stunned, &BlowerInit, &Blowing, &BlowerWait, &TreeInit, &TreeSwaying, &TreeWait, &Inactive};
    return all;
}

/** 113b:2363 - Objects.kt objectsUpdate: every enemy's state. */
void updateEnemies(Game& game) {
    for (int i = 0; i < game.world.enemyCount; i++) {
        EnemyTurn turn(game, i);
        if (turn.e.state && turn.e.kind) turn.e.state->update(turn);
        else game.problems.push_back("enemy " + std::to_string(i) + " without a state or kind");
    }
}

}  // namespace ugh
