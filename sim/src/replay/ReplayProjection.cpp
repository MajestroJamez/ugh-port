#include "replay/ReplayProjection.hpp"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <optional>
#include <type_traits>

#include "bonuses/BonusStates.hpp"
#include "core/Random.hpp"
#include "enemies/EnemyStates.hpp"
#include "passengers/PassengerStates.hpp"

// TODO(9c): one field list per entity with a Visitor instead of the templates and std::function below.

namespace ugh::replay {

namespace {

using core::Fixed;
using core::Word;

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
        case Format::Signed: return std::to_string(Word(word).value());
        case Format::Unsigned: return std::to_string(Word(word).bits());
        case Format::Byte: return std::to_string(word & 0xff);
        case Format::Hex: return hex(word);
        case Format::HexOrNone: return Word(word).bits() == 0xffff ? "none" : hex(word);
        case Format::HexOrNone0: return Word(word).bits() == 0 ? "none" : hex(word);
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
int toWord(Word v) { return v.value(); }
int toWord(Fixed v) { return v.raw().value(); }
int toWord(core::Speed v) { return v.raw().value(); }
int toWord(uint8_t v) { return v; }
int toWord(uint16_t v) { return v; }
int toWord(int v) { return v; }
int toWord(const model::PassengerCounter& v) { return v.word().value(); }
int toWord(const model::PassengerTimer& v) { return v.word().value(); }
int toWord(const model::EnemyTimer& v) { return v.word().value(); }
int toWord(const model::EnemyFacing& v) { return v.word().value(); }
int toWord(const model::BonusTimer& v) { return v.word().value(); }

template <class M>
M fromWord(int v) {
    if constexpr (std::is_same_v<M, Word>) return Word(v);
    else if constexpr (std::is_same_v<M, int>) return Word(v).value();
    else if constexpr (std::is_class_v<M>) return M(Word(v));   // Fixed, Speed and the shared words
    else return static_cast<M>(v);
}

/** What setting a field needs besides the value. */
struct Context {
    const data::GameData& data;
    int fill;
};

/** A field of the replay state on T: its text, set from a text (false: a bad value), filled with a pattern. */
template <class T>
struct Field {
    const char* name;
    std::function<std::optional<std::string>(const T&)> get;
    std::function<bool(T&, const std::string&, const Context&)> set;
    std::function<void(T&, int)> fill;
};

/** A plain value: `at` returns a reference to it. */
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

/**
 * A slot index the logic uses (0 .. slots - 1); another value is refused. The pattern is a valid index too
 * (0 or 1), so even a core that never got the value works with a slot.
 */
template <class T>
Field<T> slotIndex(const char* name, int T::*member, int slots) {
    return {name,
            [=](const T& t) -> std::optional<std::string> { return std::to_string(t.*member); },
            [=](T& t, const std::string& text, const Context&) {
                auto v = parseNumber(text, 10);
                if (!v || *v < 0 || *v >= slots) return false;
                t.*member = *v;
                return true;
            },
            [=](T& t, int fill) { t.*member = fill & 1; }};
}

/** The animation frame and its countdown: the original counts the frame in bytes, two per frame. */
template <class T>
Field<T> animationFrame(model::Animator T::*member) {
    return {"anim",
            [=](const T& t) -> std::optional<std::string> { return std::to_string(Word((t.*member).frame() << 1).value()); },
            [=](T& t, const std::string& text, const Context&) {
                auto v = parseNumber(text, 10);
                if (!v || *v % 2 != 0) return false;
                t.*member = model::Animator(*v / 2, (t.*member).delay());
                return true;
            },
            [=](T& t, int fill) { t.*member = model::Animator(fill, (t.*member).delay()); }};
}

template <class T>
Field<T> animationDelay(model::Animator T::*member) {
    return {"animDelay",
            [=](const T& t) -> std::optional<std::string> { return format(Format::Signed, (t.*member).delay().value()); },
            [=](T& t, const std::string& text, const Context&) {
                auto v = parseNumber(text, 10);
                if (v) t.*member = model::Animator((t.*member).frame(), *v);
                return v.has_value();
            },
            [=](T& t, int fill) { t.*member = model::Animator((t.*member).frame(), fill); }};
}

/** A state, by its name. */
template <class T, class S>
Field<T> state(const S* T::*member, const std::vector<const S*>& (*all)()) {
    return {"state",
            [=](const T& t) -> std::optional<std::string> {
                if (!(t.*member)) return std::nullopt;
                return std::string((t.*member)->name());
            },
            [=](T& t, const std::string& text, const Context&) {
                for (const S* s : all()) {
                    if (text == s->name()) { t.*member = s; return true; }
                }
                return false;
            },
            [=](T& t, int) { t.*member = nullptr; }};
}

/** A kind, by the original offset of its descriptor. */
template <class T, class K>
Field<T> kind(const K* T::*member, const K* (data::GameData::*lookup)(uint16_t) const) {
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
std::string rainChecksum(const model::Rain::Snapshot& rain) {
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

/** The game fields (game.*): the session, the level's values, the water, the rain. */
struct GameState {
    model::GameSession::Snapshot session;
    core::Random::Snapshot random;
    model::Level::Snapshot level;
    model::Water::Snapshot water;
    model::Rain::Snapshot rain;

    static GameState of(const model::Level& level) {
        return {level.session().snapshot(), level.session().random().snapshot(), level.snapshot(),
                level.water().snapshot(), level.rain().snapshot()};
    }

    void restoreTo(model::Level& target) const {
        target.session().restore(session);
        target.session().random().restore(random);
        target.restore(level);
        target.water().restore(water);
        target.rain().restore(rain);
    }
};

const std::vector<Field<GameState>>& gameFields() {
    using G = GameState;
    static const std::vector<Field<G>> fields = {
        value<G>("level", Format::Unsigned, [](auto& g) -> auto& { return g.session.levelNumber; }),
        value<G>("players", Format::Unsigned, [](auto& g) -> auto& { return g.session.players; }),
        {"difficulty",   // the rules have a value for each of the three
         [](const G& g) -> std::optional<std::string> { return std::to_string(g.session.difficulty.bits()); },
         [](G& g, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (!v || *v < 0 || *v >= data::GameData::DIFFICULTIES) return false;
             g.session.difficulty = *v;
             return true;
         },
         [](G& g, int fill) { g.session.difficulty = fill & 1; }},
        value<G>("lives", Format::Byte, [](auto& g) -> auto& { return g.session.lives; }),
        value<G>("multiplier", Format::Byte, [](auto& g) -> auto& { return g.session.multiplier; }),
        {"energy",
         [](const G& g) -> std::optional<std::string> { return format(Format::Signed, g.level.energy.value().value()); },
         [](G& g, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (v) g.level.energy = model::Energy(*v);
             return v.has_value();
         },
         [](G& g, int fill) { g.level.energy = model::Energy(fill); }},
        {"fade",
         [](const G& g) -> std::optional<std::string> { return format(Format::Signed, g.level.fade.position().value()); },
         [](G& g, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (v) g.level.fade = model::Fade(*v, g.level.fade.step());
             return v.has_value();
         },
         [](G& g, int fill) { g.level.fade = model::Fade(fill, g.level.fade.step()); }},
        {"fadeStep",
         [](const G& g) -> std::optional<std::string> { return format(Format::Signed, g.level.fade.step().value()); },
         [](G& g, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (v) g.level.fade = model::Fade(g.level.fade.position(), *v);
             return v.has_value();
         },
         [](G& g, int fill) { g.level.fade = model::Fade(g.level.fade.position(), fill); }},
        {"wind",   // none, to the left, to the right
         [](const G& g) -> std::optional<std::string> { return std::to_string(g.level.wind); },
         [](G& g, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (!v || *v < 0 || *v > 2) return false;
             g.level.wind = static_cast<uint8_t>(*v);
             return true;
         },
         [](G& g, int fill) { g.level.wind = static_cast<uint8_t>(fill & 2); }},
        value<G>("waterRow", Format::Signed, [](auto& g) -> auto& { return g.water.row; }),
        value<G>("passengersLeft", Format::Byte, [](auto& g) -> auto& { return g.level.passengersLeft; }),
        value<G>("waterYf", Format::Signed, [](auto& g) -> auto& { return g.water.level; }),
        value<G>("waterHold", Format::Byte, [](auto& g) -> auto& { return g.water.hold; }),
        value<G>("waterToggle", Format::Byte, [](auto& g) -> auto& { return g.water.toggle; }),
        value<G>("waterAnim", Format::Signed, [](auto& g) -> auto& { return g.water.surfaceFrame; }),
        value<G>("waterAnimDelay", Format::Byte, [](auto& g) -> auto& { return g.water.surfaceDelay; }),
        {"levelDone",
         [](const G& g) -> std::optional<std::string> { return g.level.done ? "1" : "0"; },
         [](G& g, const std::string& text, const Context&) {
             if (text != "0" && text != "1") return false;
             g.level.done = text == "1";
             return true;
         },
         [](G& g, int fill) { g.level.done = fill != 0; }},
        {"rainFloor",   // the original keeps the byte offset of the row in the VGA page
         [](const G& g) -> std::optional<std::string> { return std::to_string(g.rain.floorRow * 96); },
         [](G& g, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (!v || *v % 96 != 0 || *v / 96 > 0xff) return false;
             g.rain.floorRow = static_cast<uint8_t>(*v / 96);
             return true;
         },
         [](G& g, int fill) { g.rain.floorRow = static_cast<uint8_t>(fill); }},
        {"score",
         [](const G& g) -> std::optional<std::string> { return std::to_string(g.session.score); },
         [](G& g, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (v) g.session.score = static_cast<uint32_t>(*v);
             return v.has_value();
         },
         [](G& g, int fill) { g.session.score = static_cast<uint16_t>(fill) * 0x10001u; }},
        {"rng",
         [](const G& g) -> std::optional<std::string> {
             const auto& s = g.random;
             char buf[20];
             std::snprintf(buf, sizeof buf, "%04x%04x%04x%04x", s[3], s[2], s[1], s[0]);
             return buf;
         },
         [](G& g, const std::string& text, const Context&) {
             if (text.size() != 16) return false;
             for (int i = 0; i < 4; i++) {
                 auto v = parseNumber(text.substr(4 * i, 4), 16);
                 if (!v) return false;
                 g.random[3 - i] = static_cast<uint16_t>(*v);
             }
             return true;
         },
         [](G& g, int fill) { g.random.fill(static_cast<uint16_t>(fill)); }},
        {"rain",   // the replay has only a checksum of the drops: the core keeps its own while they match it
         [](const G& g) -> std::optional<std::string> { return g.level.wind == 0 ? "none" : rainChecksum(g.rain); },
         [](G& g, const std::string& text, const Context& c) {
             if (text != "none" && text != rainChecksum(g.rain)) g.rain.drops.fill(c.fill);
             return true;
         },
         [](G&, int) {}},
    };
    return fields;
}

const std::vector<Field<model::Copter::Snapshot>>& copterFields() {
    using C = model::Copter::Snapshot;
    static const std::vector<Field<C>> fields = {
        value("xf", Format::Signed, &C::x),
        value("yf", Format::Signed, &C::y),
        value("vx", Format::Signed, &C::vx),
        value("vy", Format::Signed, &C::vy),
        {"landedPad",   // a pad slot, or in the air
         [](const C& c) -> std::optional<std::string> { return std::to_string(c.landedPad); },
         [](C& c, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (!v || *v < model::Copter::IN_THE_AIR || *v >= model::Level::PADS) return false;
             c.landedPad = *v;
             return true;
         },
         [](C& c, int fill) { c.landedPad = Word(fill).value(); }},
        value("effort", Format::Signed, &C::effort),
        value("impact", Format::Signed, &C::impact),
        value("carrying", Format::HexOrNone0, &C::carrying),
        value("targetPad", Format::Signed, &C::targetPad),
        value("fare", Format::Signed, &C::fare),
        value("fareMin", Format::Signed, &C::fareMin),
        value("x", Format::Signed, &C::pixelX),
        value("y", Format::Signed, &C::pixelY),
        value("sprite", Format::Hex, &C::rotor),
        value("animCounter", Format::Signed, &C::rotorCounter),
        {"keys",
         [](const C& c) -> std::optional<std::string> {
             std::string keys;
             if (c.keys.up) keys += 'U';
             if (c.keys.down) keys += 'D';
             if (c.keys.left) keys += 'L';
             if (c.keys.right) keys += 'R';
             if (c.keys.fire) keys += 'F';
             return keys.empty() ? "-" : keys;
         },
         [](C& c, const std::string& text, const Context&) {
             auto held = [&](char letter) { return text.find(letter) != std::string::npos; };
             c.keys = {held('U'), held('D'), held('L'), held('R'), held('F')};
             return true;
         },
         [](C& c, int fill) { c.keys = {fill != 0, fill != 0, fill != 0, fill != 0, fill != 0}; }},
    };
    return fields;
}

const std::vector<Field<model::Pad::Snapshot>>& padFields() {
    using P = model::Pad::Snapshot;
    static const std::vector<Field<P>> fields = {
        value<P>("left", Format::Signed, [](auto& p) -> auto& { return p.place.left; }),
        value<P>("right", Format::Signed, [](auto& p) -> auto& { return p.place.right; }),
        value<P>("y", Format::Signed, [](auto& p) -> auto& { return p.place.y; }),
        value<P>("number", Format::Signed, [](auto& p) -> auto& { return p.place.number; }),
        value<P>("doorX", Format::Signed, [](auto& p) -> auto& { return p.place.doorX; }),
        value<P>("waitX", Format::Signed, [](auto& p) -> auto& { return p.place.waitX; }),
        value<P>("standX", Format::Signed, [](auto& p) -> auto& { return p.place.standX; }),
        {"waiting",   // the original keeps the passenger's offset in its arrays (passenger * 2)
         [](const P& p) -> std::optional<std::string> {
             return std::to_string(p.waiting == model::Pad::NOBODY ? -1 : Word(p.waiting * 2).value());
         },
         [](P& p, const std::string& text, const Context&) {
             auto v = parseNumber(text, 10);
             if (!v || (*v != -1 && *v % 2 != 0)) return false;
             p.waiting = *v == -1 ? model::Pad::NOBODY : *v / 2;
             return true;
         },
         [](P& p, int fill) { p.waiting = Word(fill).value(); }},
    };
    return fields;
}

const std::vector<Field<model::Passenger::Snapshot>>& passengerFields() {
    using P = model::Passenger::Snapshot;
    static const std::vector<Field<P>> fields = {
        kind(&P::kind, &data::GameData::passengerKind),
        state(&P::state, passengers::PassengerStates::all),
        value("xf", Format::Signed, &P::x),
        value("yf", Format::Signed, &P::y),
        value("vy", Format::Signed, &P::vy),
        value("timer", Format::Signed, &P::timer),
        value("counter", Format::Signed, &P::counter),
        slotIndex("pickupPad", &P::pickupPad, model::Level::PADS),
        slotIndex("targetPad", &P::targetPad, model::Level::PADS),
        value("bonusTimer", Format::Signed, &P::bonusTimer),
        value("sprite", Format::HexOrNone, &P::sprite),
        value("bubble", Format::HexOrNone, &P::bubble),
        value("startPad", Format::Signed, &P::startPad),
        value("x", Format::Signed, &P::pixelX),
        value("y", Format::Signed, &P::pixelY),
        animationFrame(&P::animator),
        animationDelay(&P::animator),
        {"route",   // the original keeps a pointer that moves 4 bytes per stop
         [](const P& p) -> std::optional<std::string> {
             return hex(p.route.route ? p.route.route->origin + 4 * p.route.stop : 0);
         },
         [](P& p, const std::string& text, const Context& c) {
             auto v = parseHex(text);
             if (!v) return false;
             if (*v == 0) { p.route = {}; return true; }
             return c.data.routeAt(static_cast<uint16_t>(*v), p.route);
         },
         [](P& p, int) { p.route = {}; }},
    };
    return fields;
}

const char* enemyName(data::EnemyKind::Type type) {
    switch (type) {
        case data::EnemyKind::Type::Flyer: return "flyer";
        case data::EnemyKind::Type::Walker: return "walker";
        case data::EnemyKind::Type::Blower: return "blower";
        case data::EnemyKind::Type::Tree: return "tree";
    }
    return "";
}

const std::vector<Field<model::Enemy::Snapshot>>& enemyFields() {
    using E = model::Enemy::Snapshot;
    static const std::vector<Field<E>> fields = {
        {"kind",   // by name
         [](const E& e) -> std::optional<std::string> {
             if (!e.kind) return std::nullopt;
             return std::string(enemyName(e.kind->type));
         },
         [](E& e, const std::string& text, const Context& c) {
             for (auto type : {data::EnemyKind::Type::Flyer, data::EnemyKind::Type::Walker, data::EnemyKind::Type::Blower,
                               data::EnemyKind::Type::Tree}) {
                 const data::EnemyKind* kind = c.data.enemyKind(type);
                 if (kind && text == enemyName(type)) { e.kind = kind; return true; }
             }
             return false;
         },
         [](E& e, int) { e.kind = nullptr; }},
        state(&E::state, enemies::EnemyStates::all),
        value("xf", Format::Signed, &E::x),
        value("yf", Format::Signed, &E::y),
        value("vx", Format::Signed, &E::vx),
        value("timer", Format::Signed, &E::timer),
        value("facing", Format::Signed, &E::facing),
        value("sprite", Format::HexOrNone, &E::sprite),
        slotIndex("pad", &E::pad, model::Level::PADS),
        value("startDelay", Format::Signed, &E::startDelay),
        animationFrame(&E::animator),
        animationDelay(&E::animator),
        {"table",   // a pointer: the flyer's animation, the tree's next bonus item, or what an earlier enemy left
         [](const E& e) -> std::optional<std::string> {
             if (e.table.isFlightAnimation()) {
                 const data::Animation* a = e.table.flightAnimation();
                 return hex(a ? a->origin() : 0);
             }
             if (const data::DropCursor* d = e.table.dropCursor()) return hex(d->list->origin + 2 * d->index);
             return hex(*e.table.leftoverWord());
         },
         [](E& e, const std::string& text, const Context& c) {
             auto v = parseHex(text);
             if (!v) return false;
             auto address = static_cast<uint16_t>(*v);
             data::DropCursor drops;
             if (const data::Animation* a = c.data.animation(address)) e.table = model::EnemyTable::flight(a);
             else if (c.data.dropsAt(address, drops)) e.table = model::EnemyTable::drops(drops);
             else e.table = model::EnemyTable::leftover(address);
             return true;
         },
         [](E& e, int fill) { e.table = model::EnemyTable::leftover(static_cast<uint16_t>(fill)); }},
    };
    return fields;
}

const std::vector<Field<model::BonusItem::Snapshot>>& bonusFields() {
    using B = model::BonusItem::Snapshot;
    static const std::vector<Field<B>> fields = {
        kind(&B::kind, &data::GameData::bonusKind),
        state(&B::state, bonuses::BonusStates::all),
        value("xf", Format::Signed, &B::x),
        value("yf", Format::Signed, &B::y),
        value("vx", Format::Signed, &B::timer),
        value("vy", Format::Signed, &B::vy),
        value("sprite", Format::Hex, &B::sprite),
    };
    return fields;
}

template <class T>
int setField(const std::vector<Field<T>>& fields, T& snapshot, const std::string& name, const std::string& text,
             const Context& c) {
    for (const auto& f : fields)
        if (name == f.name) return f.set(snapshot, text, c) ? 1 : -1;
    return 0;
}

/** Sets a field of an entity through its snapshot. */
template <class Entity>
int setEntityField(const std::vector<Field<typename Entity::Snapshot>>& fields, Entity& entity, const std::string& name,
                   const std::string& text, const Context& c) {
    typename Entity::Snapshot snapshot = entity.snapshot();
    int result = setField(fields, snapshot, name, text, c);
    if (result == 1) entity.restore(snapshot);
    return result;
}

template <class T>
void listFields(const std::vector<Field<T>>& fields, const T& snapshot, const std::string& prefix,
                std::vector<std::pair<std::string, std::string>>& out) {
    for (const auto& f : fields)
        if (auto v = f.get(snapshot)) out.emplace_back(prefix + f.name, *v);
}

template <class Entity>
void fillEntity(const std::vector<Field<typename Entity::Snapshot>>& fields, Entity& entity, int fill) {
    typename Entity::Snapshot snapshot = entity.snapshot();
    for (const auto& f : fields) f.fill(snapshot, fill);
    entity.restore(snapshot);
}

}  // namespace

void ReplayProjection::reset(model::Level& level, int fill) {
    model::Rain::Snapshot rain = level.rain().snapshot();
    rain.drops.fill(fill);
    level.rain().restore(rain);
    forget(level, fill);
}

void ReplayProjection::forget(model::Level& level, int fill) {
    fill_ = fill;
    GameState game = GameState::of(level);
    for (const auto& f : gameFields()) f.fill(game, fill);
    game.level.padCount = game.level.passengerCount = game.level.enemyCount = 0;
    game.restoreTo(level);
    for (int i = 0; i < model::Level::COPTERS; i++) fillEntity(copterFields(), level.copter(i), fill);
    for (int i = 0; i < model::Level::PADS; i++) fillEntity(padFields(), level.pad(i), fill);
    for (int i = 0; i < model::Level::PASSENGERS; i++) fillEntity(passengerFields(), level.passenger(i), fill);
    for (int i = 0; i < model::Level::ENEMIES; i++) fillEntity(enemyFields(), level.enemy(i), fill);
    for (int i = 0; i < model::BonusSlots::SLOTS; i++) {
        model::BonusItem& item = level.bonuses()[i];
        fillEntity(bonusFields(), item, fill);
        item.disappear();
    }
}

int ReplayProjection::set(model::Level& level, const std::string& field, const std::string& value) {
    Context c{data_, fill_};
    auto dot = field.find('.');
    if (dot == std::string::npos) return 0;
    std::string group = field.substr(0, dot), rest = field.substr(dot + 1);
    if (group == "game") {
        GameState game = GameState::of(level);
        int result = setField(gameFields(), game, rest, value, c);
        if (result == 1) game.restoreTo(level);
        return result;
    }
    auto dot2 = rest.find('.');
    if (dot2 == std::string::npos) return 0;
    auto index = parseNumber(rest.substr(0, dot2), 10);
    if (!index || *index < 0) return 0;
    int i = *index;
    std::string name = rest.substr(dot2 + 1);
    // a field of a list entry makes the list that long
    auto grow = [&](int model::Level::Snapshot::*count, int slots) {
        if (i >= slots) return false;
        model::Level::Snapshot s = level.snapshot();
        s.*count = std::max(s.*count, i + 1);
        level.restore(s);
        return true;
    };
    if (group == "copter")
        return i < model::Level::COPTERS ? setEntityField(copterFields(), level.copter(i), name, value, c) : 0;
    if (group == "pad")
        return grow(&model::Level::Snapshot::padCount, model::Level::PADS)
                   ? setEntityField(padFields(), level.pad(i), name, value, c) : 0;
    if (group == "passenger")
        return grow(&model::Level::Snapshot::passengerCount, model::Level::PASSENGERS)
                   ? setEntityField(passengerFields(), level.passenger(i), name, value, c) : 0;
    if (group == "object")
        return grow(&model::Level::Snapshot::enemyCount, model::Level::ENEMIES)
                   ? setEntityField(enemyFields(), level.enemy(i), name, value, c) : 0;
    if (group == "bonus")
        return i < model::BonusSlots::SLOTS ? setEntityField(bonusFields(), level.bonuses()[i], name, value, c) : 0;
    return 0;
}

std::vector<std::pair<std::string, std::string>> ReplayProjection::fields(const model::Level& level) const {
    std::vector<std::pair<std::string, std::string>> out;
    listFields(gameFields(), GameState::of(level), "game.", out);
    auto prefix = [](const char* group, int i) { return std::string(group) + "." + std::to_string(i) + "."; };
    int copters = std::clamp<int>(level.session().players().bits(), 1, 2);
    for (int i = 0; i < copters; i++) listFields(copterFields(), level.copter(i).snapshot(), prefix("copter", i), out);
    for (int i = 0; i < level.padCount(); i++) listFields(padFields(), level.pad(i).snapshot(), prefix("pad", i), out);
    for (int i = 0; i < level.passengerCount(); i++)
        listFields(passengerFields(), level.passenger(i).snapshot(), prefix("passenger", i), out);
    for (int i = 0; i < level.enemyCount(); i++)
        listFields(enemyFields(), level.enemy(i).snapshot(), prefix("object", i), out);
    for (int i = 0; i < model::BonusSlots::SLOTS; i++) {
        const model::BonusItem& item = level.bonuses()[i];
        if (item.inUse()) listFields(bonusFields(), item.snapshot(), prefix("bonus", i), out);
    }
    return out;
}

}  // namespace ugh::replay
