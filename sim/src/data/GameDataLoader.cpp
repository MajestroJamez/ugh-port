#include "data/GameDataLoader.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include "core/AmigaScale.hpp"

namespace ugh::data {

namespace {

using core::Fixed;
using core::Word;

// the tables of the original's data segment (DGROUP offsets)
constexpr int LEVEL_TABLES = 0x2909;        // one player, team: the level records in play order
constexpr int LEVEL_COUNTS = 0x2624;
constexpr int MULTIPLIER_LIMITS = 0x2628;   // by difficulty
constexpr int CRASH_LIMITS = 0x262e;        // by difficulty
constexpr int ROTOR_SPRITES = 0x0cfa;       // player 0, player 1, the end of player 1
constexpr int KEY_TABLE = 0x281c;           // 6 bytes per key, up to 0xff
constexpr int BACKGROUND_ROW_BYTES = 0x00c3;
constexpr uint16_t QUICK_DELIVERY_BONUS = 0x7a38;

/**
 * The state handlers (113b:xxxx) a descriptor names in its slots +08 .. +2c, as the state machines of the core
 * have them hard-wired. A slot nobody jumps to is 0 here and not checked.
 */
const std::vector<uint16_t> WALKING_SLOTS = {0x149c, 0x1509, 0x153b, 0x1582, 0x15b4, 0x15d7, 0x16f6, 0x172a, 0x17e6,
                                             0x180a, 0x18c8, 0x18e6, 0x19fb, 0x1a42, 0x1a7e, 0x1b29, 0x1bbe, 0x1bd6,
                                             0x1c0e};
const std::vector<uint16_t> SWIMMING_SLOTS = {0x1da8, 0x1dd5, 0x1e9c, 0x1ec0, 0x1f24, 0x1f43, 0x16f6, 0x1fe2, 0x17e6,
                                              0x2068, 0x18c8, 0x20c1, 0x19e0, 0, 0, 0, 0, 0, 0x1c0e};
const std::vector<uint16_t> STANDING_SLOTS = {0, 0, 0, 0, 0x1c0f, 0x1c27, 0, 0, 0, 0, 0, 0, 0x1c48, 0x1c6b, 0x1c81,
                                              0x1cee, 0, 0, 0x1c0e};
const std::vector<uint16_t> FLYER_SLOTS = {0x2379, 0x239f, 0x23b0, 0x23d9, 0x23ea, 0x2493, 0, 0, 0, 0, 0x252b,
                                           0x255e, 0x1c0e};
const std::vector<uint16_t> WALKER_SLOTS = {0x25b1, 0x25c9, 0x2667, 0x2681, 0x272e, 0x2748, 0x2830, 0x2844, 0x288f,
                                            0, 0x28ee, 0x2914, 0x1c0e};
const std::vector<uint16_t> BLOWER_SLOTS = {0, 0, 0, 0, 0x295b, 0x2973, 0, 0, 0, 0, 0x2a53, 0x2a76, 0x1c0e};
const std::vector<uint16_t> TREE_SLOTS = {0x2a87, 0x2ab5, 0, 0, 0, 0, 0, 0, 0, 0, 0x2b0c, 0x2b58, 0x2b7e};
const std::vector<uint16_t> BONUS_SLOTS = {0x2b96, 0x2be4, 0x2c97, 0x2ca9};   // +04 spawn, +06 falling, +08 landed, +0a lying

constexpr uint16_t STANDING_PASSENGER = 0x78dc;
constexpr uint16_t UNRESCUABLE_SWIMMER = 0x77fe;   // 113b:1f43 checks it by address
constexpr uint16_t FLYER = 0x7630, BLOWER = 0x76a8, TREE = 0x76e4;   // any other enemy type is a walker (113b:3b21)

// the slots of the level lists of the core (the original's arrays)
constexpr size_t MAX_PADS = 10, MAX_PASSENGERS = 16, MAX_ENEMIES = 5;

struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::string hex(int v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04x", v & 0xffff);
    return buf;
}

}  // namespace

std::unique_ptr<const GameData> GameDataLoader::load(const std::string& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { error = "cannot open " + path; return nullptr; }
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto u16at = [&](size_t o) { return static_cast<uint32_t>(file[o] | (file[o + 1] << 8)); };
    auto u32at = [&](size_t o) { return u16at(o) | (u16at(o + 2) << 16); };
    if (file.size() < 12 || std::memcmp(file.data(), "UGHSIM01", 8) != 0) { error = path + " is not a UGHSIM01 file"; return nullptr; }
    uint32_t count = u32at(8);
    if (file.size() < 12 + 20ull * count) { error = path + ": truncated block table"; return nullptr; }

    auto data = std::unique_ptr<GameData>(new GameData());
    Masks masks;
    for (uint32_t i = 0; i < count; i++) {
        size_t e = 12 + 20ull * i;
        std::string name(reinterpret_cast<const char*>(&file[e]), strnlen(reinterpret_cast<const char*>(&file[e]), 8));
        uint32_t offset = u16at(e + 10), start = u32at(e + 12), length = u32at(e + 16);
        if (start + static_cast<uint64_t>(length) > file.size()) { error = path + ": block " + name + " outside the file"; return nullptr; }
        if (name == "DGROUP") {
            if (!data->image_.place(static_cast<int>(offset), file.data() + start, length)) { error = "DGROUP block too large"; return nullptr; }
        } else if (name == "MASK") {
            if (length != CollisionMask::WIDTH / 8 * CollisionMask::HEIGHT) { error = "collision mask of an unexpected size"; return nullptr; }
            masks[static_cast<uint16_t>(offset)] = std::vector<uint8_t>(file.begin() + start, file.begin() + start + length);
        }
        // MAPS (the tile maps) and SPRITES are for the renderer
    }
    try {
        GameDataLoader(*data).build(masks);
    } catch (const Failure& f) {
        error = path + ": " + f.what();
        return nullptr;
    }
    return data;
}

void GameDataLoader::build(Masks& masks) {
    if (byte(BACKGROUND_ROW_BYTES) != 0x60) fail("the background page is not 96 bytes per row");
    for (int mode = 0; mode < GameData::PLAYERS; mode++) {
        int table = word(LEVEL_TABLES + 2 * mode), count = word(LEVEL_COUNTS + 2 * mode);
        for (int i = 0; i < count; i++) d_.order_[mode].push_back(&level(word(table + 2 * i), masks));
    }
    for (int difficulty = 0; difficulty < GameData::DIFFICULTIES; difficulty++) {
        d_.crashLimits_[difficulty] = signedWord(CRASH_LIMITS + 2 * difficulty);
        d_.multiplierLimits_[difficulty] = signedWord(MULTIPLIER_LIMITS + 2 * difficulty);
    }
    for (int i = 0; i <= GameData::PLAYERS; i++) d_.rotorSprites_[i] = word(ROTOR_SPRITES + 2 * i);
    d_.quickDeliveryBonus_ = &bonusKind(QUICK_DELIVERY_BONUS);
    for (int entry = KEY_TABLE; byte(entry) != 0xff; entry += 6) d_.keys_.push_back(key(entry));
}

void GameDataLoader::fail(const std::string& what) { throw Failure(what); }

void GameDataLoader::checkSlots(uint16_t descriptor, const std::vector<uint16_t>& slots, const char* what) const {
    for (size_t i = 0; i < slots.size(); i++) {
        int slot = 8 + 2 * static_cast<int>(i);
        if (slots[i] != 0 && word(descriptor + slot) != slots[i])
            fail(std::string(what) + " " + hex(descriptor) + ": slot +" + hex(slot) + " is not 113b:" + hex(slots[i]));
    }
}

const Animation* GameDataLoader::animation(uint16_t origin) {
    if (origin == 0) return nullptr;
    auto& a = d_.animations_[origin];
    if (!a) a = std::make_unique<Animation>(d_.image_, origin);
    return a.get();
}

AnimationPair GameDataLoader::pair(int offset) { return {animation(word(offset)), animation(word(offset + 2))}; }

Box GameDataLoader::box(uint16_t descriptor) const {
    return {signedWord(descriptor), signedWord(descriptor + 2), signedWord(descriptor + 4), signedWord(descriptor + 6)};
}

const PassengerKind& GameDataLoader::passengerKind(uint16_t origin) {
    auto& slot = d_.passengerKinds_[origin];
    if (slot) return *slot;
    slot = std::make_unique<PassengerKind>();
    PassengerKind& k = *slot;
    k.origin = origin;
    if (word(origin + 8) == WALKING_SLOTS[0]) {
        k.type = PassengerKind::Type::Walking;
        checkSlots(origin, WALKING_SLOTS, "walking passenger");
    } else if (word(origin + 8) == SWIMMING_SLOTS[0]) {
        k.type = PassengerKind::Type::Swimming;
        checkSlots(origin, SWIMMING_SLOTS, "swimming passenger");
    } else {
        k.type = PassengerKind::Type::Standing;
        checkSlots(origin, STANDING_SLOTS, "standing passenger");
    }
    k.box = box(origin);
    k.standing = animation(word(origin + 0x2e));
    k.waving = animation(word(origin + 0x30));
    k.walking = pair(origin + 0x32);
    k.appearing = animation(word(origin + 0x36));
    k.entering = animation(word(origin + 0x38));
    k.animDelay = signedWord(origin + 0x3e);
    k.fare = signedWord(origin + 0x40);
    k.fareMin = signedWord(origin + 0x42);
    k.swimTime = signedWord(origin + 0x44);
    k.look = signedWord(origin + 0x46);
    k.rescuable = origin != UNRESCUABLE_SWIMMER;
    // a walking kind turns into a swimming one in the water and back; the standing ones have each other
    k.other = &passengerKind(word(origin + 0x48));
    bool standing = k.type == PassengerKind::Type::Standing;
    bool otherStanding = k.other->type == PassengerKind::Type::Standing;
    if (standing != otherStanding || (!standing && k.other->type == k.type))
        fail("passenger " + hex(origin) + ": +48 is not its other kind");
    if (!standing && (!k.standing || !k.waving || !k.walking.left || !k.walking.right))
        fail("passenger " + hex(origin) + ": an animation is missing");
    if (k.type == PassengerKind::Type::Walking && (!k.appearing || !k.entering))
        fail("passenger " + hex(origin) + ": a door animation is missing");
    return k;
}

const BonusKind& GameDataLoader::bonusKind(uint16_t origin) {
    auto& slot = d_.bonusKinds_[origin];
    if (slot) return *slot;
    slot = std::make_unique<BonusKind>();
    BonusKind& k = *slot;
    k.origin = origin;
    for (size_t i = 0; i < BONUS_SLOTS.size(); i++)
        if (word(origin + 4 + 2 * static_cast<int>(i)) != BONUS_SLOTS[i]) fail("bonus item " + hex(origin) + ": unexpected state handlers");
    k.x = signedWord(origin);
    k.y = signedWord(origin + 2);
    k.lift = signedWord(origin + 0x0c);
    k.amount = signedWord(origin + 0x0e);
    k.sprite = word(origin + 0x10);
    switch (word(origin + 0x12)) {
        case 0: k.effect = BonusKind::Effect::Energy; break;
        case 1: k.effect = BonusKind::Effect::Life; break;
        case 2: k.effect = BonusKind::Effect::Multiplier; break;
        default: fail("bonus item " + hex(origin) + ": unknown effect");
    }
    return k;
}

const EnemyKind& GameDataLoader::enemyKind(uint16_t origin) {
    auto& slot = d_.enemyKinds_[origin];
    if (slot) return *slot;
    slot = std::make_unique<EnemyKind>();
    EnemyKind& k = *slot;
    k.origin = origin;
    switch (origin) {
        case FLYER: k.type = EnemyKind::Type::Flyer; checkSlots(origin, FLYER_SLOTS, "flyer"); break;
        case BLOWER: k.type = EnemyKind::Type::Blower; checkSlots(origin, BLOWER_SLOTS, "blower"); break;
        case TREE: k.type = EnemyKind::Type::Tree; checkSlots(origin, TREE_SLOTS, "tree"); break;
        default:
            // the replays name the kinds, so there can be only one walker
            for (const auto& [other, kind] : d_.enemyKinds_)
                if (other != origin && kind && kind->type == EnemyKind::Type::Walker) fail("a second walker kind " + hex(origin));
            k.type = EnemyKind::Type::Walker;
            checkSlots(origin, WALKER_SLOTS, "walker");
    }
    k.box = box(origin);
    k.moving = pair(origin + 0x22);
    k.charging = pair(origin + 0x26);
    k.recovering = pair(origin + 0x2a);
    k.watching = pair(origin + 0x2e);
    k.stunned = pair(origin + 0x36);
    k.score = signedWord(origin + 0x3a);
    return k;
}

const Route& GameDataLoader::route(uint16_t origin) {
    auto& r = d_.routes_[origin];
    if (r) return *r;
    r = std::make_unique<Route>();
    r->origin = origin;
    for (int a = origin;; a += 4) {
        r->words.push_back(signedWord(a));
        if (word(a) == DataImage::LIST_END) break;
        r->words.push_back(signedWord(a + 2));
        if (r->words.size() > 64) fail("route " + hex(origin) + " has no end");
    }
    if (r->words.size() < 3) fail("route " + hex(origin) + " has no stop");
    return *r;
}

const DropList& GameDataLoader::drops(uint16_t origin) {
    auto& l = d_.drops_[origin];
    if (l) return *l;
    l = std::make_unique<DropList>();
    l->origin = origin;
    for (int a = origin; word(a) != DataImage::LIST_END; a += 2) l->items.push_back(&bonusKind(word(a)));
    return *l;
}

/** 113b:3976 - Level.kt loadLevel: the record and its lists A (pads), B (passengers), C (enemies). */
const LevelDefinition& GameDataLoader::level(uint16_t record, Masks& masks) {
    auto& slot = d_.levels_[record];
    if (slot) return *slot;
    slot = std::make_unique<LevelDefinition>();
    LevelDefinition& l = *slot;
    l.origin = record;
    loadPads(l, word(record + 2));
    loadPassengers(l, word(record + 4));
    loadEnemies(l, word(record + 6));
    l.toDeliver = byte(record + 8);
    l.wind = byte(record + 0x0c);
    if (l.wind > 2) fail("level " + hex(record) + ": unknown wind");
    for (int p = 0; p < GameData::PLAYERS; p++) {
        l.startX[p] = Fixed(signedWord(record + 0x0d + 4 * p) - 0x200);
        l.startY[p] = Fixed(core::amigaRowsToPc(signedWord(record + 0x0f + 4 * p)) - 0x140);
    }
    Word water = core::amigaRowsToPc(signedWord(record + 0x15));
    if (Word::unsignedLess(0x17e0, water)) water = 0x17e0;
    l.water = Fixed(water);
    l.waterSpeed = signedWord(record + 0x17);
    auto mask = masks.find(record);
    if (mask == masks.end()) fail("level " + hex(record) + ": no collision mask");
    l.mask = CollisionMask(std::move(mask->second));
    masks.erase(mask);
    return l;
}

void GameDataLoader::loadPads(LevelDefinition& l, int list) {
    for (int entry = list; signedWord(entry) >= 0; entry += 16) {
        PadDefinition p;
        p.left = signedWord(entry);
        p.right = signedWord(entry + 2);
        p.y = core::amigaRowsToPc(signedWord(entry + 4));
        p.doorX = signedWord(entry + 6);
        p.waitX = signedWord(entry + 8);
        p.standX = signedWord(entry + 10);
        p.number = signedWord(entry + 12);
        l.pads.push_back(p);
    }
    if (l.pads.size() >= MAX_PADS) fail("level " + hex(l.origin) + ": too many pads");
}

void GameDataLoader::loadPassengers(LevelDefinition& l, int list) {
    auto pad = [&](int index) -> const PadDefinition& {
        if (index < 0 || index >= static_cast<int>(l.pads.size())) fail("level " + hex(l.origin) + ": no pad " + std::to_string(index));
        return l.pads[index];
    };
    for (int entry = list; word(entry) != DataImage::LIST_END; entry += 6) {
        PassengerPlacement p;
        p.kind = &passengerKind(word(entry));
        p.pad = signedWord(entry + 2).value();
        if (p.kind->type == PassengerKind::Type::Standing) {
            if (word(entry) != STANDING_PASSENGER) fail("level " + hex(l.origin) + ": a standing passenger of another kind");
            if (word(entry + 4) != 0) fail("level " + hex(l.origin) + ": a standing passenger with a route");
            p.x = Fixed::fromPixels(pad(p.pad).standX - 8);
            p.y = Fixed::fromPixels(pad(p.pad).y - 0xb);
        } else {
            if (p.kind->type != PassengerKind::Type::Walking) fail("level " + hex(l.origin) + ": a swimming passenger");
            p.route = {&route(word(entry + 4)), 0};
            // every stop goes between pads of the level
            for (RouteCursor stop = p.route; !stop.finished(); stop.stop++) {
                pad(stop.from());
                pad(stop.to());
            }
        }
        l.passengers.push_back(p);
    }
    if (l.passengers.size() >= MAX_PASSENGERS) fail("level " + hex(l.origin) + ": too many passengers");
}

void GameDataLoader::loadEnemies(LevelDefinition& l, int list) {
    auto pad = [&](int index) -> const PadDefinition& {
        if (index < 0 || index >= static_cast<int>(l.pads.size())) fail("level " + hex(l.origin) + ": no pad " + std::to_string(index));
        return l.pads[index];
    };
    for (int entry = list; word(entry) != DataImage::LIST_END;) {
        EnemyPlacement e;
        e.kind = &enemyKind(word(entry));
        switch (e.kind->type) {
            case EnemyKind::Type::Tree:
                e.pad = signedWord(entry + 2).value();
                e.x = Fixed::fromPixels(pad(e.pad).standX - 0x10);
                e.y = Fixed::fromPixels(pad(e.pad).y - 0x18);
                e.drops = &drops(word(entry + 4));
                entry += 6;
                break;
            case EnemyKind::Type::Flyer:
                e.startDelay = core::amigaFramesToPc(signedWord(entry + 4));
                e.vx = Fixed(signedWord(entry + 6));
                entry += 8;
                break;
            case EnemyKind::Type::Blower:
                e.x = Fixed::fromPixels(signedWord(entry + 2));
                e.y = Fixed(core::amigaRowsToPc(Fixed::fromPixels(signedWord(entry + 4)).raw()) - 0x2c0);
                entry += 10;
                break;
            case EnemyKind::Type::Walker: {
                e.pad = signedWord(entry + 2).value();
                e.x = Fixed::fromPixels(pad(e.pad).standX - 0x10);
                e.y = Fixed::fromPixels(pad(e.pad).y - 0x16);
                Word speed = signedWord(entry + 6);
                e.vx = Fixed((speed >> 2) - speed);
                entry += 8;
            }
        }
        l.enemies.push_back(e);
    }
    if (l.enemies.size() >= MAX_ENEMIES) fail("level " + hex(l.origin) + ": too many enemies");
}

KeyBinding GameDataLoader::key(int entry) const {
    KeyBinding k;
    k.scancodes = {byte(entry), byte(entry + 1)};
    if (byte(entry + 2) != KeyBinding::NO_SCANCODE) fail("key table: a sequence of three scancodes");
    int slot = byte(entry + 3);   // the offset of the key state in the table at 278c: 4 per key, 2 per player
    if (slot == 0x14) {
        k.key = KeyBinding::Key::None;
    } else if (slot < 0x14 && slot % 2 == 0) {
        k.key = static_cast<KeyBinding::Key>(slot / 4);
        k.player = (slot / 2) & 1;
    } else {
        fail("key table: unknown key " + hex(slot));
    }
    if (byte(entry + 4) != 0 && byte(entry + 4) != 0xff) fail("key table: a key state other than 0 / 0xff");
    k.press = byte(entry + 4) == 0xff;
    return k;
}

}  // namespace ugh::data
