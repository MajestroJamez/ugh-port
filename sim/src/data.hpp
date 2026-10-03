// The data of the game logic, read by a factory from the extractor's export (assets/sim/ugh-sim.bin, format
// UGHSIM01, re/notes/phase2-data.md) into types: levels, pads, kinds of passengers, enemies and bonus items,
// animations, routes, the key table, the collision masks.
//
// Only this layer and the replay projection know the offsets of the original's data segment (DGROUP). The objects
// keep theirs as `origin`: the identity the golden replays use (passenger.N.kind=0x7720, route=0x3cf9 ...).
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "fixed.hpp"

namespace ugh {

struct PassengerState;
struct EnemyState;
struct BonusState;
struct EnemyBehavior;

/** A sprite number (assets/sprites); NO_SPRITE = nothing drawn. */
using Sprite = uint16_t;
constexpr Sprite NO_SPRITE = 0xffff;

/** The end of the word lists in the data (animations, routes, bonus drops). */
constexpr uint16_t LIST_END = 0xffff;

/** The initialized data segment of the original as the export has it; the lists are read from it. */
class Image {
public:
    std::vector<uint8_t> bytes = std::vector<uint8_t>(0x10000);
    uint16_t word(int offset) const;
};

/**
 * An animation: sprite numbers up to LIST_END. The original reads past the end into whatever follows when an
 * animation position comes from a longer one, so frame() reads the data like the original does.
 */
class Animation {
public:
    Animation(const Image& image, uint16_t origin) : image_(&image), origin_(origin) {}
    uint16_t origin() const { return origin_; }
    Sprite frame(int index) const { return image_->word(origin_ + 2 * index); }

private:
    const Image* image_;
    uint16_t origin_;
};

/** Left and right variant of an animation (chosen by facing, see Enemy::facing). */
struct AnimationPair {
    const Animation* left = nullptr;
    const Animation* right = nullptr;
    const Animation* facing(int16_t facing) const { return facing == 0 ? left : right; }
};

/**
 * Where a sprite stands and how big it is for collisions (descriptor +0 .. +6): x / y from the sprite's top left
 * to the point that stands on a pad (x the middle, y the feet), and half the width and height of the touch box.
 */
struct Box {
    int16_t x = 0, y = 0, halfWidth = 0, halfHeight = 0;
};

/** The three state machines a passenger kind belongs to (they share the state handler slots of the descriptor). */
enum class PassengerSet { Walking, Swimming, Standing };

/** A kind of passenger (descriptor at 7720, 776a, 77b4 ... 0x4a bytes each). */
struct PassengerKind {
    uint16_t origin = 0;
    PassengerSet set = PassengerSet::Walking;
    Box box;
    const Animation* standing = nullptr;    // +2e (swimming: treading water)
    const Animation* waving = nullptr;      // +30
    AnimationPair walking;                  // +32 / +34
    const Animation* appearing = nullptr;   // +36: out of the door
    const Animation* entering = nullptr;    // +38: into the door
    int16_t animDelay = 0;                  // +3e: frames per animation step
    int16_t fare = 0, fareMin = 0;          // +40 / +42: the fare when boarding, it drops to the minimum
    int16_t swimTime = 0;                   // +44: frames a swimmer stays afloat
    int16_t look = 0;                       // +46: who sits in the copter (Copter::carrying)
    const PassengerKind* other = nullptr;   // +48: the swimming kind of a walking one and back
    bool rescuable = true;                  // a swimmer a copter on the water can pick up (all but 77fe)
};

/** A kind of bonus item (descriptor at 7970, 7984 ... and 7a38, 0x14 bytes each). */
struct BonusKind {
    enum class Effect { Energy, Life, Multiplier };

    uint16_t origin = 0;
    int16_t x = 0, y = 0;    // +0 / +2: from the top left to the middle / the bottom (lands on pads)
    int16_t lift = 0;        // +0c: upward speed when dropped
    int16_t amount = 0;      // +0e: energy or lives
    Sprite sprite = 0;       // +10
    Effect effect = Effect::Energy;   // +12
};

/** A kind of enemy (descriptor at 7630 flyer, 766c walker, 76a8 blower, 76e4 tree, 0x3c bytes each). */
struct EnemyKind {
    uint16_t origin = 0;
    const char* name = "";          // as in the replays: flyer, walker, blower, tree
    const EnemyBehavior* behavior = nullptr;
    Box box;
    // the animations of the descriptor (+22 .. +38); every kind uses some of them
    AnimationPair moving;      // +22 / +24: flyer flight, walker walk, tree sway (left)
    AnimationPair charging;    // +26 / +28: walker charge, blower blowing (left)
    AnimationPair recovering;  // +2a / +2c: walker
    AnimationPair watching;    // +2e / +30: walker
    AnimationPair stunned;     // +36 / +38: flyer hit, walker and blower stunned
    int16_t score = 0;         // +3a: for stunning it with a passenger
};

/** The route of a walking passenger: pairs (pad, delay) up to LIST_END (level list B). */
struct Route {
    uint16_t origin = 0;
    std::vector<int16_t> words;
};

/** A position in a route: stop n goes from pad words[2n] (after delay words[2n+1]) to pad words[2n+2]. */
struct RouteCursor {
    const Route* route = nullptr;   // nullptr: no route (the standing passenger)
    int stop = 0;

    bool finished() const { return to() == static_cast<int16_t>(LIST_END); }
    int16_t from() const { return route->words[2 * stop]; }
    int16_t delay() const { return route->words[2 * stop + 1]; }
    int16_t to() const { return route->words[2 * stop + 2]; }
};

/** The bonus items a tree drops, one per hit (level list C). */
struct DropList {
    uint16_t origin = 0;
    std::vector<const BonusKind*> items;
};

/** The next bonus item of a tree. */
struct DropCursor {
    const DropList* list = nullptr;
    int index = 0;

    bool finished() const { return index >= static_cast<int>(list->items.size()); }
    const BonusKind& next() const { return *list->items[index]; }
};

/**
 * Collision mask of a level: bit 7 of the background page's colours, 384 x 192 px. The original probes the page
 * in VGA memory, so a probe point is a linear pixel index (y * 384 + x) that runs into the next or previous row
 * at the edges; outside the page nothing is solid (re/notes/phase2-data.md).
 */
class CollisionMask {
public:
    static constexpr int WIDTH = 384, HEIGHT = 192;

    CollisionMask() = default;
    explicit CollisionMask(std::vector<uint8_t> bits) : bits_(std::move(bits)) {}

    bool solid(int index) const;

private:
    std::vector<uint8_t> bits_;   // 48 bytes per row, pixel x in bit 7 - (x & 7) of byte x / 8
};

/** A pad (level list A); the position values are in pixels. */
struct Pad {
    int16_t left = 0, right = 0;   // the landing area
    int16_t y = 0;                 // the surface
    int16_t doorX = 0;             // where passengers come out and go in
    int16_t waitX = 0;             // where they wait
    int16_t standX = 0;            // where an enemy or the standing passenger stands
    int16_t number = 0;            // shown in the bubbles and the status line
    int16_t waiting = -1;          // the passenger waiting there, -1 = none
};

/** A passenger of a level (list B). */
struct PassengerPlacement {
    const PassengerKind* kind = nullptr;
    int16_t pad = 0;
    RouteCursor route;
    Fixed x, y;                    // the standing passenger only
};

/** An enemy of a level (list C); which values a kind sets is up to its behavior. */
struct EnemyPlacement {
    const EnemyKind* kind = nullptr;
    int16_t pad = 0;
    Fixed x, y;
    int16_t startDelay = 0;
    Fixed vx;
    const DropList* drops = nullptr;
};

/** A level (record of 26 bytes, tables 3349 one player / 33d5 team; 113b:3976 loads it). */
struct LevelDefinition {
    uint16_t origin = 0;
    std::vector<Pad> pads;
    std::vector<PassengerPlacement> passengers;
    std::vector<EnemyPlacement> enemies;
    uint8_t toDeliver = 0;        // +08: passengers to deliver
    uint8_t wind = 0;             // +0c: 0 none, 1 to the left, 2 to the right; rain with it
    std::array<Fixed, 2> startX, startY;   // +0d .. +13: copters
    Fixed water;                  // +15: water surface
    int16_t waterSpeed = 0;       // +17: added every second frame
    CollisionMask mask;
};

/** A key of the keyboard table (DGROUP:281c): a scancode or an 0xE0-prefixed pair, and what it does. */
struct KeyBinding {
    enum class Key { Up, Down, Left, Right, Fire, None };

    std::array<uint8_t, 2> scancodes{};   // second 0xff: a single scancode
    int player = 0;
    Key key = Key::None;                  // None: swallowed (the fake shifts of extended keys)
    bool press = false;                   // press or release
};

/** All the data, made by load(). */
class GameData {
public:
    /** The factory: reads and checks the export; nullptr with the reason in `error`. */
    static std::unique_ptr<const GameData> load(const std::string& path, std::string& error);

    /** Level `number` (from 0) of the one-player (1) or team (2) order; nullptr when there is none. */
    const LevelDefinition* level(int players, int number) const;
    int levelCount(int players) const;

    /** Impact that crashes a copter (DGROUP:262e, by difficulty; read like the original for any value). */
    int16_t crashLimit(int difficulty) const { return static_cast<int16_t>(image_.word(0x262e + 2 * difficulty)); }
    /** Highest score multiplier (DGROUP:2628, by difficulty). */
    uint16_t multiplierLimit(int difficulty) const { return image_.word(0x2628 + 2 * difficulty); }

    /** Rotor sprites of player 0 / 1: first, and one past the last (DGROUP:0cfa). */
    Sprite rotorFirst(int player) const { return image_.word(0x0cfa + 2 * player); }
    Sprite rotorEnd(int player) const { return image_.word(0x0cfc + 2 * player); }

    const std::vector<KeyBinding>& keys() const { return keys_; }
    const BonusKind& quickDeliveryBonus() const { return *bonusKinds_.at(0x7a38); }

    // lookups by origin (the replay projection)
    const PassengerKind* passengerKind(uint16_t origin) const;
    const EnemyKind* enemyKind(uint16_t origin) const;
    const BonusKind* bonusKind(uint16_t origin) const;
    const Animation* animation(uint16_t origin) const;
    bool routeAt(uint16_t address, RouteCursor& cursor) const;
    bool dropsAt(uint16_t address, DropCursor& cursor) const;
    const std::map<uint16_t, std::unique_ptr<EnemyKind>>& enemyKinds() const { return enemyKinds_; }

private:
    class Factory;

    Image image_;
    std::map<uint16_t, std::unique_ptr<Animation>> animations_;
    std::map<uint16_t, std::unique_ptr<PassengerKind>> passengerKinds_;
    std::map<uint16_t, std::unique_ptr<EnemyKind>> enemyKinds_;
    std::map<uint16_t, std::unique_ptr<BonusKind>> bonusKinds_;
    std::map<uint16_t, std::unique_ptr<Route>> routes_;
    std::map<uint16_t, std::unique_ptr<DropList>> drops_;
    std::map<uint16_t, std::unique_ptr<LevelDefinition>> levels_;   // by record
    std::array<std::vector<const LevelDefinition*>, 2> order_;     // one player, team
    std::vector<KeyBinding> keys_;
};

}  // namespace ugh
