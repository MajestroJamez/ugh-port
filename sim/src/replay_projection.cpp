#include "replay_projection.hpp"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <optional>
#include <type_traits>

#include "bonuses.hpp"
#include "enemies.hpp"
#include "passengers.hpp"

namespace ugh {

namespace {

/** How a replay writes a value. */
enum class Format { Signed, Unsigned, Byte, Hex, HexOrNone, HexOrNone0 };

std::string hex(int v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%04x", v & 0xffff);
    return buf;
}

std::optional<int> parseNumber(const std::string& text, int base) {
    try {
        size_t used = 0;
        long long v = std::stoll(text, &used, base);
        if (used != text.size()) return std::nullopt;
        return static_cast<int>(v);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int> parseHex(const std::string& text) {
    if (text.rfind("0x", 0) != 0) return std::nullopt;
    return parseNumber(text.substr(2), 16);
}

std::string format(Format f, int word) {
    switch (f) {
        case Format::Signed: return std::to_string(static_cast<int16_t>(word));
        case Format::Unsigned: return std::to_string(static_cast<uint16_t>(word));
        case Format::Byte: return std::to_string(static_cast<uint8_t>(word));
        case Format::Hex: return hex(word);
        case Format::HexOrNone: return static_cast<uint16_t>(word) == 0xffff ? "none" : hex(word);
        case Format::HexOrNone0: return static_cast<uint16_t>(word) == 0 ? "none" : hex(word);
    }
    return "";
}

std::optional<int> parse(Format f, const std::string& text) {
    switch (f) {
        case Format::Signed: case Format::Unsigned: case Format::Byte: return parseNumber(text, 10);
        case Format::Hex: return parseHex(text);
        case Format::HexOrNone: return text == "none" ? std::optional<int>(0xffff) : parseHex(text);
        case Format::HexOrNone0: return text == "none" ? std::optional<int>(0) : parseHex(text);
    }
    return std::nullopt;
}

// a model value as the word of the original, and back
int toWord(int16_t v) { return v; }
int toWord(uint16_t v) { return v; }
int toWord(uint8_t v) { return v; }
int toWord(Fixed v) { return v.raw(); }

template <class M>
M fromWord(int v) {
    if constexpr (std::is_same_v<M, Fixed>) return Fixed(v);
    else return static_cast<M>(v);
}

/** What setting a field needs besides the value. */
struct Context {
    const GameData& data;
    int fill;
};

/** A field of the replay state on an entity T: its text, set from a text (false: a bad value), filled with a pattern. */
template <class T>
struct Field {
    const char* name;
    std::function<std::optional<std::string>(const T&)> get;
    std::function<bool(T&, const std::string&, const Context&)> set;
    std::function<void(T&, int)> fill;
};

/** A plain value: `at` returns a reference to it in the entity. */
template <class T, class At>
Field<T> value(const char* name, Format f, At at) {
    using M = std::remove_cvref_t<decltype(at(std::declval<T&>()))>;
    return {name,
            [=](const T& t) -> std::optional<std::string> { return format(f, toWord(at(t))); },
            [=](T& t, const std::string& text, const Context&) {
                auto v = parse(f, text);
                if (v) at(t) = fromWord<M>(*v);
                return v.has_value();
            },
            [=](T& t, int fill) { at(t) = fromWord<M>(fill); }};
}

template <class T, class M>
Field<T> value(const char* name, Format f, M T::*member) {
    return value<T>(name, f, [member](auto& t) -> auto& { return t.*member; });
}

/** An animation position: the original counts it in bytes, two per frame. */
template <class T>
Field<T> animationPosition(const char* name, int16_t T::*member) {
    return {name,
            [=](const T& t) -> std::optional<std::string> { return std::to_string(static_cast<int16_t>(t.*member * 2)); },
            [=](T& t, const std::string& text, const Context&) {
                auto v = parseNumber(text, 10);
                if (!v || *v % 2 != 0) return false;
                t.*member = static_cast<int16_t>(*v / 2);
                return true;
            },
            [=](T& t, int fill) { t.*member = static_cast<int16_t>(fill); }};
}

/** A state, by its name. */
template <class T, class S>
Field<T> state(const S* T::*member, const std::vector<const S*>& (*all)()) {
    return {"state",
            [=](const T& t) -> std::optional<std::string> {
                if (!(t.*member)) return std::nullopt;
                return std::string((t.*member)->name);
            },
            [=](T& t, const std::string& text, const Context&) {
                for (const S* s : all()) {
                    if (text == s->name) { t.*member = s; return true; }
                }
                return false;
            },
            [=](T& t, int) { t.*member = nullptr; }};
}

/** A kind, by the original offset of its descriptor. */
template <class T, class K>
Field<T> kind(const K* T::*member, const K* (GameData::*lookup)(uint16_t) const) {
    return {"kind",
            [=](const T& t) -> std::optional<std::string> {
                if (!(t.*member)) return std::nullopt;
                return hex((t.*member)->origin);
            },
            [=](T& t, const std::string& text, const Context& c) {
                auto v = parseHex(text);
                const K* k = v ? (c.data.*lookup)(static_cast<uint16_t>(*v)) : nullptr;
                if (k) t.*member = k;
                return k != nullptr;
            },
            [=](T& t, int) { t.*member = nullptr; }};
}

/** CRC-32 (zlib) as in StateProjection.rainChecksum. */
uint32_t crc32(const std::vector<uint8_t>& bytes) {
    uint32_t crc = 0xffffffffu;
    for (uint8_t b : bytes) {
        crc ^= b;
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
    }
    return ~crc;
}

/** The raindrops as the original keeps them (a byte offset in the VGA page and a plane, 2e8b / 318f), checksummed. */
std::string rainChecksum(const Rain& rain) {
    std::vector<uint8_t> bytes;
    for (int32_t drop : rain.drops) {
        for (int v : {drop >> 2, drop & 3}) {
            bytes.push_back(static_cast<uint8_t>(v));
            bytes.push_back(static_cast<uint8_t>(v >> 8));
        }
    }
    char buf[16];
    std::snprintf(buf, sizeof buf, "%08x", crc32(bytes));
    return buf;
}

const std::vector<Field<World>>& gameFields() {
    static const std::vector<Field<World>> fields = {
        value<World>("level", Format::Unsigned, &World::levelNumber),
        value<World>("players", Format::Unsigned, &World::players),
        value<World>("difficulty", Format::Unsigned, &World::difficulty),
        value<World>("lives", Format::Byte, &World::lives),
        value<World>("multiplier", Format::Byte, &World::multiplier),
        value<World>("energy", Format::Signed, &World::energy),
        value<World>("fade", Format::Signed, [](auto& w) -> auto& { return w.fade.position; }),
        value<World>("fadeStep", Format::Signed, [](auto& w) -> auto& { return w.fade.step; }),
        value<World>("wind", Format::Byte, &World::wind),
        value<World>("waterRow", Format::Signed, [](auto& w) -> auto& { return w.water.row; }),
        value<World>("passengersLeft", Format::Byte, &World::passengersLeft),
        value<World>("waterYf", Format::Signed, [](auto& w) -> auto& { return w.water.level; }),
        value<World>("waterHold", Format::Byte, [](auto& w) -> auto& { return w.water.hold; }),
        value<World>("waterToggle", Format::Byte, [](auto& w) -> auto& { return w.water.toggle; }),
        value<World>("waterAnim", Format::Signed, [](auto& w) -> auto& { return w.water.surfaceFrame; }),
        value<World>("waterAnimDelay", Format::Byte, [](auto& w) -> auto& { return w.water.surfaceDelay; }),
        {"levelDone",
         [](const World& w) -> std::optional<std::string> { return w.levelDone ? "1" : "0"; },
         [](World& w, const std::string& text, const Context&) {
             if (text != "0" && text != "1") return false;
             w.levelDone = text == "1";
             return true;
         },
         [](World& w, int fill) { w.levelDone = fill != 0; }},
        {"rainFloor",   // the original keeps the byte offset of the row in the VGA page
         [](const World& w) -> std::optional<std::string> { return std::to_string(w.rain.floorRow * 96); },
         [](World& w, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (!v || *v % 96 != 0 || *v / 96 > 0xff) return false;
             w.rain.floorRow = static_cast<uint8_t>(*v / 96);
             return true;
         },
         [](World& w, int fill) { w.rain.floorRow = static_cast<uint8_t>(fill); }},
        {"score",
         [](const World& w) -> std::optional<std::string> { return std::to_string(w.score); },
         [](World& w, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (v) w.score = static_cast<uint32_t>(*v);
             return v.has_value();
         },
         [](World& w, int fill) { w.score = static_cast<uint16_t>(fill) * 0x10001u; }},
        {"rng",
         [](const World& w) -> std::optional<std::string> {
             const auto& s = w.random.state;
             char buf[20];
             std::snprintf(buf, sizeof buf, "%04x%04x%04x%04x", s[3], s[2], s[1], s[0]);
             return buf;
         },
         [](World& w, const std::string& text, const Context&) {
             if (text.size() != 16) return false;
             for (int i = 0; i < 4; i++) {
                 auto v = parseNumber(text.substr(4 * i, 4), 16);
                 if (!v) return false;
                 w.random.state[3 - i] = static_cast<uint16_t>(*v);
             }
             return true;
         },
         [](World& w, int fill) { w.random.state.fill(static_cast<uint16_t>(fill)); }},
        {"rain",   // the replay has only a checksum of the drops: the core keeps its own while they match it
         [](const World& w) -> std::optional<std::string> { return w.wind == 0 ? "none" : rainChecksum(w.rain); },
         [](World& w, const std::string& text, const Context& c) {
             if (text != "none" && text != rainChecksum(w.rain)) w.rain.drops.fill(c.fill);
             return true;
         },
         [](World&, int) {}},
    };
    return fields;
}

const std::vector<Field<Copter>>& copterFields() {
    static const std::vector<Field<Copter>> fields = {
        value("xf", Format::Signed, &Copter::x),
        value("yf", Format::Signed, &Copter::y),
        value("vx", Format::Signed, &Copter::vx),
        value("vy", Format::Signed, &Copter::vy),
        value("landedPad", Format::Signed, &Copter::landedPad),
        value("effort", Format::Signed, &Copter::effort),
        value("impact", Format::Signed, &Copter::impact),
        value("carrying", Format::HexOrNone0, &Copter::carrying),
        value("targetPad", Format::Signed, &Copter::targetPad),
        value("fare", Format::Signed, &Copter::fare),
        value("fareMin", Format::Signed, &Copter::fareMin),
        value("x", Format::Signed, &Copter::pixelX),
        value("y", Format::Signed, &Copter::pixelY),
        value("sprite", Format::Hex, &Copter::rotor),
        value("animCounter", Format::Signed, &Copter::rotorCounter),
        {"keys",
         [](const Copter& c) -> std::optional<std::string> {
             std::string keys;
             if (c.keys.up) keys += 'U';
             if (c.keys.down) keys += 'D';
             if (c.keys.left) keys += 'L';
             if (c.keys.right) keys += 'R';
             if (c.keys.fire) keys += 'F';
             return keys.empty() ? "-" : keys;
         },
         [](Copter& c, const std::string& text, const Context&) {
             auto held = [&](char letter) { return text.find(letter) != std::string::npos; };
             c.keys = {held('U'), held('D'), held('L'), held('R'), held('F')};
             return true;
         },
         [](Copter& c, int fill) { c.keys = {fill != 0, fill != 0, fill != 0, fill != 0, fill != 0}; }},
    };
    return fields;
}

const std::vector<Field<Pad>>& padFields() {
    static const std::vector<Field<Pad>> fields = {
        value("left", Format::Signed, &Pad::left),
        value("right", Format::Signed, &Pad::right),
        value("y", Format::Signed, &Pad::y),
        value("number", Format::Signed, &Pad::number),
        value("doorX", Format::Signed, &Pad::doorX),
        value("waitX", Format::Signed, &Pad::waitX),
        value("standX", Format::Signed, &Pad::standX),
        {"waiting",   // the original keeps the passenger's offset in its arrays (passenger * 2)
         [](const Pad& p) -> std::optional<std::string> {
             return std::to_string(p.waiting == -1 ? -1 : static_cast<int16_t>(p.waiting * 2));
         },
         [](Pad& p, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (!v || (*v != -1 && *v % 2 != 0)) return false;
             p.waiting = static_cast<int16_t>(*v == -1 ? -1 : *v / 2);
             return true;
         },
         [](Pad& p, int fill) { p.waiting = static_cast<int16_t>(fill); }},
    };
    return fields;
}

const std::vector<Field<Passenger>>& passengerFields() {
    static const std::vector<Field<Passenger>> fields = {
        kind(&Passenger::kind, &GameData::passengerKind),
        state(&Passenger::state, passengerStates),
        value("xf", Format::Signed, &Passenger::x),
        value("yf", Format::Signed, &Passenger::y),
        value("vy", Format::Signed, &Passenger::vy),
        value("timer", Format::Signed, &Passenger::timer),
        value("counter", Format::Signed, &Passenger::counter),
        value("pickupPad", Format::Signed, &Passenger::pickupPad),
        value("targetPad", Format::Signed, &Passenger::targetPad),
        value("bonusTimer", Format::Signed, &Passenger::bonusTimer),
        value("sprite", Format::HexOrNone, &Passenger::sprite),
        value("bubble", Format::HexOrNone, &Passenger::bubble),
        value("startPad", Format::Signed, &Passenger::startPad),
        value("x", Format::Signed, &Passenger::pixelX),
        value("y", Format::Signed, &Passenger::pixelY),
        animationPosition("anim", &Passenger::anim),
        value("animDelay", Format::Signed, &Passenger::animDelay),
        {"route",   // the original keeps a pointer that moves 4 bytes per stop
         [](const Passenger& p) -> std::optional<std::string> {
             return hex(p.route.route ? p.route.route->origin + 4 * p.route.stop : 0);
         },
         [](Passenger& p, const std::string& text, const Context& c) {
             auto v = parseHex(text);
             if (!v) return false;
             if (*v == 0) { p.route = {}; return true; }
             return c.data.routeAt(static_cast<uint16_t>(*v), p.route);
         },
         [](Passenger& p, int) { p.route = {}; }},
    };
    return fields;
}

const std::vector<Field<Enemy>>& enemyFields() {
    static const std::vector<Field<Enemy>> fields = {
        {"kind",   // by name
         [](const Enemy& e) -> std::optional<std::string> {
             if (!e.kind) return std::nullopt;
             return std::string(e.kind->name);
         },
         [](Enemy& e, const std::string& text, const Context& c) {
             for (const auto& [origin, k] : c.data.enemyKinds()) {
                 if (text == k->name) { e.kind = k.get(); return true; }
             }
             return false;
         },
         [](Enemy& e, int) { e.kind = nullptr; }},
        state(&Enemy::state, enemyStates),
        value("xf", Format::Signed, &Enemy::x),
        value("yf", Format::Signed, &Enemy::y),
        value("vx", Format::Signed, &Enemy::vx),
        value("timer", Format::Signed, &Enemy::timer),
        value("facing", Format::Signed, &Enemy::facing),
        value("sprite", Format::HexOrNone, &Enemy::sprite),
        value("pad", Format::Signed, &Enemy::pad),
        value("startDelay", Format::Signed, &Enemy::startDelay),
        animationPosition("anim", &Enemy::anim),
        value("animDelay", Format::Signed, &Enemy::animDelay),
        {"table",   // a pointer: the flyer's animation, the tree's next bonus item, or what an earlier enemy left
         [](const Enemy& e) -> std::optional<std::string> {
             if (auto a = std::get_if<const Animation*>(&e.table)) return hex(*a ? (*a)->origin() : 0);
             if (auto d = std::get_if<DropCursor>(&e.table)) return hex(d->list->origin + 2 * d->index);
             return hex(std::get<Leftover>(e.table).word);
         },
         [](Enemy& e, const std::string& text, const Context& c) {
             auto v = parseHex(text);
             if (!v) return false;
             auto address = static_cast<uint16_t>(*v);
             DropCursor drops;
             if (const Animation* a = c.data.animation(address)) e.table = a;
             else if (c.data.dropsAt(address, drops)) e.table = drops;
             else e.table = Leftover{address};
             return true;
         },
         [](Enemy& e, int fill) { e.table = Leftover{static_cast<uint16_t>(fill)}; }},
    };
    return fields;
}

const std::vector<Field<BonusItem>>& bonusFields() {
    static const std::vector<Field<BonusItem>> fields = {
        kind(&BonusItem::kind, &GameData::bonusKind),
        state(&BonusItem::state, bonusStates),
        value("xf", Format::Signed, &BonusItem::x),
        value("yf", Format::Signed, &BonusItem::y),
        value("vx", Format::Signed, &BonusItem::vx),
        value("vy", Format::Signed, &BonusItem::vy),
        value("sprite", Format::Hex, &BonusItem::sprite),
    };
    return fields;
}

template <class T>
int setField(const std::vector<Field<T>>& fields, T& entity, const std::string& name, const std::string& text, const Context& c) {
    for (const auto& f : fields)
        if (name == f.name) return f.set(entity, text, c) ? 1 : -1;
    return 0;
}

template <class T>
void listFields(const std::vector<Field<T>>& fields, const T& entity, const std::string& prefix,
                std::vector<std::pair<std::string, std::string>>& out) {
    for (const auto& f : fields)
        if (auto v = f.get(entity)) out.emplace_back(prefix + f.name, *v);
}

template <class T, size_t N>
void fillAll(const std::vector<Field<T>>& fields, std::array<T, N>& entities, int fill) {
    for (T& e : entities)
        for (const auto& f : fields) f.fill(e, fill);
}

}  // namespace

void ReplayProjection::reset(World& world, int fill) {
    world.rain.drops.fill(fill);
    forget(world, fill);
}

void ReplayProjection::forget(World& world, int fill) {
    fill_ = fill;
    for (const auto& f : gameFields()) f.fill(world, fill);
    fillAll(copterFields(), world.copters, fill);
    fillAll(padFields(), world.pads, fill);
    fillAll(passengerFields(), world.passengers, fill);
    fillAll(enemyFields(), world.enemies, fill);
    fillAll(bonusFields(), world.bonuses, fill);
    for (BonusItem& b : world.bonuses) b.sprite = NO_SPRITE;
    world.padCount = world.passengerCount = world.enemyCount = 0;
    world.level = data_.level(world.players, world.levelNumber);
}

int ReplayProjection::set(World& world, const std::string& field, const std::string& value) {
    Context c{data_, fill_};
    auto dot = field.find('.');
    if (dot == std::string::npos) return 0;
    std::string group = field.substr(0, dot), rest = field.substr(dot + 1);
    if (group == "game") {
        int result = setField(gameFields(), world, rest, value, c);
        world.level = data_.level(world.players, world.levelNumber);
        return result;
    }
    auto dot2 = rest.find('.');
    if (dot2 == std::string::npos) return 0;
    auto index = parseNumber(rest.substr(0, dot2), 10);
    if (!index || *index < 0) return 0;
    int i = *index;
    std::string name = rest.substr(dot2 + 1);
    // a field of a list entry makes the list that long
    auto grow = [&](int& count, size_t slots) {
        if (i >= static_cast<int>(slots)) return false;
        count = std::max(count, i + 1);
        return true;
    };
    if (group == "copter") return i < 2 ? setField(copterFields(), world.copters[i], name, value, c) : 0;
    if (group == "pad") return grow(world.padCount, world.pads.size()) ? setField(padFields(), world.pads[i], name, value, c) : 0;
    if (group == "passenger")
        return grow(world.passengerCount, world.passengers.size()) ? setField(passengerFields(), world.passengers[i], name, value, c) : 0;
    if (group == "object")
        return grow(world.enemyCount, world.enemies.size()) ? setField(enemyFields(), world.enemies[i], name, value, c) : 0;
    if (group == "bonus") return i < 12 ? setField(bonusFields(), world.bonuses[i], name, value, c) : 0;
    return 0;
}

std::vector<std::pair<std::string, std::string>> ReplayProjection::fields(const World& world) const {
    std::vector<std::pair<std::string, std::string>> out;
    listFields(gameFields(), world, "game.", out);
    auto prefix = [](const char* group, int i) { return std::string(group) + "." + std::to_string(i) + "."; };
    int copters = std::clamp<int>(world.players, 1, 2);
    for (int i = 0; i < copters; i++) listFields(copterFields(), world.copters[i], prefix("copter", i), out);
    for (int i = 0; i < world.padCount; i++) listFields(padFields(), world.pads[i], prefix("pad", i), out);
    for (int i = 0; i < world.passengerCount; i++) listFields(passengerFields(), world.passengers[i], prefix("passenger", i), out);
    for (int i = 0; i < world.enemyCount; i++) listFields(enemyFields(), world.enemies[i], prefix("object", i), out);
    for (int i = 0; i < static_cast<int>(world.bonuses.size()); i++)
        if (world.bonuses[i].used()) listFields(bonusFields(), world.bonuses[i], prefix("bonus", i), out);
    return out;
}

}  // namespace ugh
