// The factory of the game data: reads the extractor's export "UGHSIM01" (extractor/src/main/kotlin/ugh/extractor/
// Sim.kt) and turns the original's tables into the types of data.hpp, with the arithmetic of the level loader
// 113b:3976 (Level.kt loadLevel). It also checks that the descriptors hand their state handlers over the way the
// state machines of the core assume (passengers.cpp, enemies.cpp, bonuses.cpp).
#include "data.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include "enemies.hpp"

namespace ugh {

namespace {

std::string hex(int v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04x", v & 0xffff);
    return buf;
}

/**
 * The state handlers (113b:xxxx) a descriptor names in its slots +08 .. +2c, as the state machines of the core
 * have them hard-wired. A slot nobody jumps to is 0 here and not checked.
 */
constexpr std::array<uint16_t, 19> WALKING_SLOTS = {0x149c, 0x1509, 0x153b, 0x1582, 0x15b4, 0x15d7, 0x16f6, 0x172a,
                                                    0x17e6, 0x180a, 0x18c8, 0x18e6, 0x19fb, 0x1a42, 0x1a7e, 0x1b29,
                                                    0x1bbe, 0x1bd6, 0x1c0e};
constexpr std::array<uint16_t, 19> SWIMMING_SLOTS = {0x1da8, 0x1dd5, 0x1e9c, 0x1ec0, 0x1f24, 0x1f43, 0x16f6, 0x1fe2,
                                                     0x17e6, 0x2068, 0x18c8, 0x20c1, 0x19e0, 0, 0, 0, 0, 0, 0x1c0e};
constexpr std::array<uint16_t, 19> STANDING_SLOTS = {0, 0, 0, 0, 0x1c0f, 0x1c27, 0, 0, 0, 0, 0, 0,
                                                     0x1c48, 0x1c6b, 0x1c81, 0x1cee, 0, 0, 0x1c0e};
constexpr std::array<uint16_t, 13> FLYER_SLOTS = {0x2379, 0x239f, 0x23b0, 0x23d9, 0x23ea, 0x2493, 0, 0, 0, 0,
                                                  0x252b, 0x255e, 0x1c0e};
constexpr std::array<uint16_t, 13> WALKER_SLOTS = {0x25b1, 0x25c9, 0x2667, 0x2681, 0x272e, 0x2748, 0x2830, 0x2844,
                                                   0x288f, 0, 0x28ee, 0x2914, 0x1c0e};
constexpr std::array<uint16_t, 13> BLOWER_SLOTS = {0, 0, 0, 0, 0x295b, 0x2973, 0, 0, 0, 0, 0x2a53, 0x2a76, 0x1c0e};
constexpr std::array<uint16_t, 13> TREE_SLOTS = {0x2a87, 0x2ab5, 0, 0, 0, 0, 0, 0, 0, 0, 0x2b0c, 0x2b58, 0x2b7e};

constexpr uint16_t STANDING_PASSENGER = 0x78dc;
constexpr uint16_t UNRESCUABLE_SWIMMER = 0x77fe;   // 113b:1f43 checks it by address
constexpr uint16_t FLYER = 0x7630, BLOWER = 0x76a8, TREE = 0x76e4;   // any other enemy type is a walker (113b:3b21)

struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

}  // namespace

uint16_t Image::word(int offset) const {
    return static_cast<uint16_t>(bytes[offset & 0xffff] | (bytes[(offset + 1) & 0xffff] << 8));
}

bool CollisionMask::solid(int index) const {
    if (index < 0 || index >= WIDTH * HEIGHT || bits_.empty()) return false;
    int x = index % WIDTH, y = index / WIDTH;
    return (bits_[y * (WIDTH / 8) + x / 8] & (0x80 >> (x & 7))) != 0;
}

/** Builds the objects of GameData from the image, each once, and checks them. */
class GameData::Factory {
public:
    explicit Factory(GameData& data) : d_(data) {}

    void build(std::map<uint16_t, std::vector<uint8_t>>& masks) {
        if (b(0x00c3) != 0x60) fail("the background page is not 96 bytes per row");
        for (int mode = 0; mode < 2; mode++) {
            int table = w(0x2909 + 2 * mode), count = w(0x2624 + 2 * mode);
            for (int i = 0; i < count; i++) d_.order_[mode].push_back(&level(w(table + 2 * i), masks));
        }
        d_.bonusKinds_[0x7a38] = makeBonusKind(0x7a38);
        int si = 0x281c;
        for (; b(si) != 0xff; si += 6) d_.keys_.push_back(key(si));
    }

private:
    GameData& d_;

    [[noreturn]] static void fail(const std::string& what) { throw Failure(what); }
    uint16_t w(int offset) const { return d_.image_.word(offset); }
    int16_t s(int offset) const { return static_cast<int16_t>(w(offset)); }
    uint8_t b(int offset) const { return d_.image_.bytes[offset & 0xffff]; }

    template <size_t N>
    void checkSlots(uint16_t descriptor, const std::array<uint16_t, N>& slots, const char* what) {
        for (size_t i = 0; i < N; i++) {
            if (slots[i] != 0 && w(descriptor + 8 + 2 * static_cast<int>(i)) != slots[i])
                fail(std::string(what) + " " + hex(descriptor) + ": slot +" + hex(8 + 2 * static_cast<int>(i)) +
                     " is not 113b:" + hex(slots[i]));
        }
    }

    const Animation* animation(uint16_t origin) {
        if (origin == 0) return nullptr;
        auto& a = d_.animations_[origin];
        if (!a) a = std::make_unique<Animation>(d_.image_, origin);
        return a.get();
    }

    AnimationPair pair(int offset) { return {animation(w(offset)), animation(w(offset + 2))}; }

    Box box(uint16_t descriptor) { return {s(descriptor), s(descriptor + 2), s(descriptor + 4), s(descriptor + 6)}; }

    const PassengerKind& passengerKind(uint16_t origin) {
        auto& slot = d_.passengerKinds_[origin];
        if (slot) return *slot;
        slot = std::make_unique<PassengerKind>();
        PassengerKind& k = *slot;
        k.origin = origin;
        if (w(origin + 8) == WALKING_SLOTS[0]) {
            k.set = PassengerSet::Walking;
            checkSlots(origin, WALKING_SLOTS, "walking passenger");
        } else if (w(origin + 8) == SWIMMING_SLOTS[0]) {
            k.set = PassengerSet::Swimming;
            checkSlots(origin, SWIMMING_SLOTS, "swimming passenger");
        } else {
            k.set = PassengerSet::Standing;
            checkSlots(origin, STANDING_SLOTS, "standing passenger");
        }
        k.box = box(origin);
        k.standing = animation(w(origin + 0x2e));
        k.waving = animation(w(origin + 0x30));
        k.walking = pair(origin + 0x32);
        k.appearing = animation(w(origin + 0x36));
        k.entering = animation(w(origin + 0x38));
        k.animDelay = s(origin + 0x3e);
        k.fare = s(origin + 0x40);
        k.fareMin = s(origin + 0x42);
        k.swimTime = s(origin + 0x44);
        k.look = s(origin + 0x46);
        k.rescuable = origin != UNRESCUABLE_SWIMMER;
        k.other = &passengerKind(w(origin + 0x48));
        bool walkingOrSwimming = k.set != PassengerSet::Standing;
        if ((k.other->set != PassengerSet::Standing) != walkingOrSwimming || (walkingOrSwimming && k.other->set == k.set))
            fail("passenger " + hex(origin) + ": +48 is not its other set");
        return k;
    }

    std::unique_ptr<BonusKind> makeBonusKind(uint16_t origin) {
        auto k = std::make_unique<BonusKind>();
        k->origin = origin;
        std::array<uint16_t, 4> slots = {0x2b96, 0x2be4, 0x2c97, 0x2ca9};   // +04 spawn, +06 falling, +08 landed, +0a lying
        for (int i = 0; i < 4; i++)
            if (w(origin + 4 + 2 * i) != slots[i]) fail("bonus item " + hex(origin) + ": unexpected state handlers");
        k->x = s(origin);
        k->y = s(origin + 2);
        k->lift = s(origin + 0x0c);
        k->amount = s(origin + 0x0e);
        k->sprite = w(origin + 0x10);
        switch (w(origin + 0x12)) {
            case 0: k->effect = BonusKind::Effect::Energy; break;
            case 1: k->effect = BonusKind::Effect::Life; break;
            case 2: k->effect = BonusKind::Effect::Multiplier; break;
            default: fail("bonus item " + hex(origin) + ": unknown effect");
        }
        return k;
    }

    const BonusKind& bonusKind(uint16_t origin) {
        auto& k = d_.bonusKinds_[origin];
        if (!k) k = makeBonusKind(origin);
        return *k;
    }

    const EnemyKind& enemyKind(uint16_t origin) {
        auto& slot = d_.enemyKinds_[origin];
        if (slot) return *slot;
        slot = std::make_unique<EnemyKind>();
        EnemyKind& k = *slot;
        k.origin = origin;
        switch (origin) {
            case FLYER: k.name = "flyer"; k.behavior = &Flyer; checkSlots(origin, FLYER_SLOTS, "flyer"); break;
            case BLOWER: k.name = "blower"; k.behavior = &Blower; checkSlots(origin, BLOWER_SLOTS, "blower"); break;
            case TREE: k.name = "tree"; k.behavior = &Tree; checkSlots(origin, TREE_SLOTS, "tree"); break;
            default:
                // the replays name the kinds, so there can be only one walker
                for (const auto& [other, kind] : d_.enemyKinds_)
                    if (other != origin && kind && kind->behavior == &Walker) fail("a second walker kind " + hex(origin));
                k.name = "walker";
                k.behavior = &Walker;
                checkSlots(origin, WALKER_SLOTS, "walker");
        }
        k.box = box(origin);
        k.moving = pair(origin + 0x22);
        k.charging = pair(origin + 0x26);
        k.recovering = pair(origin + 0x2a);
        k.watching = pair(origin + 0x2e);
        k.stunned = pair(origin + 0x36);
        k.score = s(origin + 0x3a);
        return k;
    }

    const Route& route(uint16_t origin) {
        auto& r = d_.routes_[origin];
        if (r) return *r;
        r = std::make_unique<Route>();
        r->origin = origin;
        for (int a = origin;; a += 4) {
            r->words.push_back(s(a));
            if (w(a) == LIST_END) break;
            r->words.push_back(s(a + 2));
            if (r->words.size() > 64) fail("route " + hex(origin) + " has no end");
        }
        if (r->words.size() < 3) fail("route " + hex(origin) + " has no stop");
        return *r;
    }

    const DropList& drops(uint16_t origin) {
        auto& l = d_.drops_[origin];
        if (l) return *l;
        l = std::make_unique<DropList>();
        l->origin = origin;
        for (int a = origin; w(a) != LIST_END; a += 2) l->items.push_back(&bonusKind(w(a)));
        return *l;
    }

    /** 113b:3976 - Level.kt loadLevel: the record and its lists A (pads), B (passengers), C (enemies). */
    const LevelDefinition& level(uint16_t record, std::map<uint16_t, std::vector<uint8_t>>& masks) {
        auto& slot = d_.levels_[record];
        if (slot) return *slot;
        slot = std::make_unique<LevelDefinition>();
        LevelDefinition& l = *slot;
        l.origin = record;

        for (int si = w(record + 2); s(si) >= 0; si += 16) {
            Pad p;
            p.left = s(si);
            p.right = s(si + 2);
            p.y = threeQuarters(s(si + 4));
            p.doorX = s(si + 6);
            p.waitX = s(si + 8);
            p.standX = s(si + 10);
            p.number = s(si + 12);
            l.pads.push_back(p);
        }
        if (l.pads.size() >= 10) fail("level " + hex(record) + ": too many pads");
        auto pad = [&](int index) -> const Pad& {
            if (index < 0 || index >= static_cast<int>(l.pads.size())) fail("level " + hex(record) + ": no pad " + std::to_string(index));
            return l.pads[index];
        };

        for (int si = w(record + 4); w(si) != LIST_END; si += 6) {
            PassengerPlacement p;
            p.kind = &passengerKind(w(si));
            p.pad = s(si + 2);
            if (p.kind->set == PassengerSet::Standing) {
                if (w(si) != STANDING_PASSENGER) fail("level " + hex(record) + ": a standing passenger of another kind");
                if (w(si + 4) != 0) fail("level " + hex(record) + ": a standing passenger with a route");
                p.x = Fixed::fromPixels(pad(p.pad).standX - 8);
                p.y = Fixed::fromPixels(pad(p.pad).y - 0xb);
            } else {
                p.route = {&route(w(si + 4)), 0};
            }
            l.passengers.push_back(p);
        }
        if (l.passengers.size() >= 16) fail("level " + hex(record) + ": too many passengers");

        for (int si = w(record + 6); w(si) != LIST_END;) {
            EnemyPlacement e;
            e.kind = &enemyKind(w(si));
            switch (e.kind->origin) {
                case TREE:
                    e.pad = s(si + 2);
                    e.x = Fixed::fromPixels(pad(e.pad).standX - 0x10);
                    e.y = Fixed::fromPixels(pad(e.pad).y - 0x18);
                    e.drops = &drops(w(si + 4));
                    si += 6;
                    break;
                case FLYER:
                    e.startDelay = threeHalves(s(si + 4));
                    e.vx = Fixed(s(si + 6));
                    si += 8;
                    break;
                case BLOWER:
                    e.x = Fixed::fromPixels(s(si + 2));
                    e.y = Fixed(threeQuarters(Fixed::fromPixels(s(si + 4)).raw()) - 0x2c0);
                    si += 10;
                    break;
                default: {
                    e.pad = s(si + 2);
                    e.x = Fixed::fromPixels(pad(e.pad).standX - 0x10);
                    e.y = Fixed::fromPixels(pad(e.pad).y - 0x16);
                    int16_t speed = s(si + 6);
                    e.vx = Fixed((speed >> 2) - speed);
                    si += 8;
                }
            }
            l.enemies.push_back(e);
        }
        if (l.enemies.size() >= 5) fail("level " + hex(record) + ": too many enemies");

        l.toDeliver = b(record + 8);
        l.wind = b(record + 0x0c);
        for (int p = 0; p < 2; p++) {
            l.startX[p] = Fixed(s(record + 0x0d + 4 * p) - 0x200);
            l.startY[p] = Fixed(threeQuarters(s(record + 0x0f + 4 * p)) - 0x140);
        }
        int16_t water = threeQuarters(s(record + 0x15));
        if (static_cast<uint16_t>(water) > 0x17e0) water = 0x17e0;
        l.water = Fixed(water);
        l.waterSpeed = s(record + 0x17);
        auto mask = masks.find(record);
        if (mask == masks.end()) fail("level " + hex(record) + ": no collision mask");
        l.mask = CollisionMask(std::move(mask->second));
        masks.erase(mask);
        return l;
    }

    KeyBinding key(int si) {
        KeyBinding k;
        k.scancodes = {b(si), b(si + 1)};
        if (b(si + 2) != 0xff) fail("key table: a sequence of three scancodes");
        int slot = b(si + 3);
        if (slot == 0x14) {
            k.key = KeyBinding::Key::None;
        } else if (slot < 0x14 && slot % 2 == 0) {
            k.key = static_cast<KeyBinding::Key>(slot / 4);
            k.player = (slot / 2) & 1;
        } else {
            fail("key table: unknown key " + hex(slot));
        }
        if (b(si + 4) != 0 && b(si + 4) != 0xff) fail("key table: a key state other than 0 / 0xff");
        k.press = b(si + 4) == 0xff;
        return k;
    }
};

std::unique_ptr<const GameData> GameData::load(const std::string& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { error = "cannot open " + path; return nullptr; }
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto u16at = [&](size_t o) { return static_cast<uint32_t>(file[o] | (file[o + 1] << 8)); };
    auto u32at = [&](size_t o) { return u16at(o) | (u16at(o + 2) << 16); };
    if (file.size() < 12 || std::memcmp(file.data(), "UGHSIM01", 8) != 0) { error = path + " is not a UGHSIM01 file"; return nullptr; }
    uint32_t count = u32at(8);
    if (file.size() < 12 + 20ull * count) { error = path + ": truncated block table"; return nullptr; }

    auto data = std::unique_ptr<GameData>(new GameData());
    std::map<uint16_t, std::vector<uint8_t>> masks;
    for (uint32_t i = 0; i < count; i++) {
        size_t e = 12 + 20ull * i;
        std::string name(reinterpret_cast<const char*>(&file[e]), strnlen(reinterpret_cast<const char*>(&file[e]), 8));
        uint32_t off = u16at(e + 10), start = u32at(e + 12), length = u32at(e + 16);
        if (start + static_cast<uint64_t>(length) > file.size()) { error = path + ": block " + name + " outside the file"; return nullptr; }
        if (name == "DGROUP") {
            if (off + static_cast<uint64_t>(length) > data->image_.bytes.size()) { error = "DGROUP block too large"; return nullptr; }
            std::copy(file.begin() + start, file.begin() + start + length, data->image_.bytes.begin() + off);
        } else if (name == "MASK") {
            if (length != CollisionMask::WIDTH / 8 * CollisionMask::HEIGHT) { error = "collision mask of an unexpected size"; return nullptr; }
            masks[static_cast<uint16_t>(off)] = std::vector<uint8_t>(file.begin() + start, file.begin() + start + length);
        }
        // MAPS (the tile maps) and SPRITES are for the renderer
    }
    try {
        Factory(*data).build(masks);
    } catch (const Failure& f) {
        error = path + ": " + f.what();
        return nullptr;
    }
    return data;
}

const LevelDefinition* GameData::level(int players, int number) const {
    if (players < 1 || players > 2) return nullptr;
    const auto& order = order_[players - 1];
    return number >= 0 && number < static_cast<int>(order.size()) ? order[number] : nullptr;
}

int GameData::levelCount(int players) const {
    return players >= 1 && players <= 2 ? static_cast<int>(order_[players - 1].size()) : 0;
}

namespace {

template <class T>
const T* find(const std::map<uint16_t, std::unique_ptr<T>>& map, uint16_t origin) {
    auto it = map.find(origin);
    return it == map.end() ? nullptr : it->second.get();
}

}  // namespace

const PassengerKind* GameData::passengerKind(uint16_t origin) const { return find(passengerKinds_, origin); }
const EnemyKind* GameData::enemyKind(uint16_t origin) const { return find(enemyKinds_, origin); }
const BonusKind* GameData::bonusKind(uint16_t origin) const { return find(bonusKinds_, origin); }
const Animation* GameData::animation(uint16_t origin) const { return find(animations_, origin); }

bool GameData::routeAt(uint16_t address, RouteCursor& cursor) const {
    // the route that starts last at or before the address (a route starting inside another one is its tail)
    auto it = routes_.upper_bound(address);
    if (it == routes_.begin()) return false;
    const Route& route = *std::prev(it)->second;
    int offset = address - route.origin;
    if (offset % 4 != 0 || 2 * (offset / 4) + 2 >= static_cast<int>(route.words.size())) return false;
    cursor = {&route, offset / 4};
    return true;
}

bool GameData::dropsAt(uint16_t address, DropCursor& cursor) const {
    for (const auto& [origin, list] : drops_) {
        int offset = address - origin;
        if (offset < 0 || offset % 2 != 0 || offset / 2 > static_cast<int>(list->items.size())) continue;
        cursor = {list.get(), offset / 2};
        return true;
    }
    return false;
}

}  // namespace ugh
