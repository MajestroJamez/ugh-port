// The replay fields as a view of the memory - the C++ side of verify/src/test/kotlin/ugh/verify/replay/
// StateProjection.kt (same names, values and formats), and what the core derives for variables the replay
// leaves out (StateProjection.NOT_PROJECTED) before it runs a transition.
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <optional>

#include "sim.hpp"

namespace ugh {

namespace {

enum class Kind { S16, U16, U8, HEX, HEX_OR_NONE, HEX_OR_NONE0, LEVEL_DONE, P_STATE, O_STATE, B_STATE, O_KIND };

struct Def {
    const char* name;
    int off;
    Kind kind;
};

int sizeOf(Kind k) { return k == Kind::U8 || k == Kind::LEVEL_DONE ? 1 : 2; }

const Def GAME[] = {
    {"level", 0x261c, Kind::U16}, {"players", 0x2634, Kind::U16}, {"difficulty", 0x2638, Kind::U16},
    {"lives", 0x263c, Kind::U8}, {"multiplier", 0x263d, Kind::U8}, {"energy", V_ENERGY, Kind::S16},
    {"fade", V_FADE, Kind::S16}, {"fadeStep", V_FADE_STEP, Kind::S16}, {"levelDone", 0x27cf, Kind::LEVEL_DONE},
    {"wind", V_WIND, Kind::U8}, {"waterRow", V_WATER_ROW, Kind::S16}, {"rainFloor", 0x2907, Kind::U16},
    // level phases
    {"passengersLeft", 0x28f1, Kind::U8}, {"waterYf", 0x28fe, Kind::S16}, {"waterHold", 0x27a2, Kind::U8},
    {"waterToggle", 0x27ce, Kind::U8}, {"waterAnim", 0x27a4, Kind::S16}, {"waterAnimDelay", 0x27a3, Kind::U8},
};
const Def COPTER[] = {
    {"xf", P_XF, Kind::S16}, {"yf", P_YF, Kind::S16}, {"vx", P_VX, Kind::S16}, {"vy", P_VY, Kind::S16},
    {"landedPad", P_LANDED, Kind::S16}, {"effort", P_EFFORT, Kind::S16}, {"impact", P_IMPACT, Kind::S16},
    {"carrying", 0x27fc, Kind::HEX_OR_NONE0}, {"targetPad", 0x2804, Kind::S16}, {"fare", 0x2808, Kind::S16},
    {"fareMin", 0x280c, Kind::S16}, {"x", P_X, Kind::S16}, {"y", P_Y, Kind::S16}, {"sprite", 0x27e8, Kind::HEX},
    {"animCounter", 0x27f0, Kind::S16},
};
const Def PAD[] = {
    {"left", PAD_LEFT, Kind::S16}, {"right", PAD_RIGHT, Kind::S16}, {"y", PAD_Y, Kind::S16},
    {"number", 0x2985, Kind::S16}, {"waiting", 0x2999, Kind::S16}, {"doorX", 0x2949, Kind::S16},
    {"waitX", 0x295d, Kind::S16}, {"standX", 0x2971, Kind::S16},
};
const Def PASSENGER[] = {
    {"kind", 0x29ad, Kind::HEX}, {"state", 0x2a0d, Kind::P_STATE}, {"xf", 0x2aed, Kind::S16},
    {"yf", 0x2b0d, Kind::S16}, {"vy", 0x2c6d, Kind::S16}, {"timer", 0x2acd, Kind::S16},
    {"counter", 0x2a2d, Kind::S16}, {"pickupPad", 0x2a4d, Kind::S16}, {"targetPad", 0x2a6d, Kind::S16},
    {"bonusTimer", 0x2c8d, Kind::S16}, {"sprite", 0x2bed, Kind::HEX_OR_NONE}, {"bubble", 0x2c2d, Kind::HEX_OR_NONE},
    {"startPad", 0x29cd, Kind::S16}, {"route", 0x29ed, Kind::HEX}, {"x", 0x2b2d, Kind::S16}, {"y", 0x2b6d, Kind::S16},
    {"anim", 0x2bad, Kind::S16}, {"animDelay", 0x2bcd, Kind::S16},
};
const Def OBJECT[] = {
    {"kind", 0x2cad, Kind::O_KIND}, {"state", 0x2cf3, Kind::O_STATE}, {"xf", 0x2cc1, Kind::S16},
    {"yf", 0x2ccb, Kind::S16}, {"vx", 0x2ce9, Kind::S16}, {"timer", 0x2cfd, Kind::S16}, {"facing", 0x2d57, Kind::S16},
    {"sprite", 0x2d43, Kind::HEX_OR_NONE}, {"pad", 0x2cb7, Kind::S16}, {"table", 0x2cd5, Kind::HEX},
    {"startDelay", 0x2cdf, Kind::S16}, {"anim", 0x2d2f, Kind::S16}, {"animDelay", 0x2d39, Kind::S16},
};
const Def BONUS[] = {
    {"kind", 0x2d6b, Kind::HEX}, {"state", 0x2d83, Kind::B_STATE}, {"xf", 0x2dcb, Kind::S16}, {"yf", 0x2de3, Kind::S16},
    {"vx", 0x2e5b, Kind::S16}, {"vy", 0x2e73, Kind::S16}, {"sprite", 0x2d9b, Kind::HEX},
};

struct Group {
    const char* name;
    const Def* defs;
    size_t count;
    int slots;
};
const Group GROUPS[] = {
    {"copter", COPTER, std::size(COPTER), 2}, {"pad", PAD, std::size(PAD), 10},
    {"passenger", PASSENGER, std::size(PASSENGER), 16}, {"object", OBJECT, std::size(OBJECT), 5},
    {"bonus", BONUS, std::size(BONUS), 12},
};

/** State handlers of the original by address (StateProjection.PASSENGER_STATES ...). */
const std::map<int, std::string> PASSENGER_STATES = {
    {0x149c, "NextStop"}, {0x1509, "Arriving"}, {0x153b, "Appear"}, {0x1582, "Appearing"},
    {0x15b4, "StartWaiting"}, {0x15d7, "Waiting"}, {0x16f6, "StartCalling"}, {0x172a, "Calling"},
    {0x17e6, "StartImpatient"}, {0x180a, "Impatient"}, {0x18c8, "StartBoarding"}, {0x18e6, "Boarding"},
    {0x19e0, "Board"}, {0x19fb, "BoardKeepDescriptor"}, {0x1a42, "Riding"}, {0x1a7e, "Paid"},
    {0x1b29, "WalkingAway"}, {0x1bbe, "StartEntering"}, {0x1bd6, "Entering"}, {0x1c0e, "Gone"},
    {0x2195, "Idle"}, {0x1c0f, "StartStanding"}, {0x1c27, "Standing"}, {0x1c48, "Grabbed"},
    {0x1c6b, "Hanging"}, {0x1c81, "Dropped"}, {0x1cee, "Falling"}, {0x1da8, "StartSplash"},
    {0x1dd5, "Splash"}, {0x1de2, "SplashAdvance"}, {0x1e9c, "StartSinking"}, {0x1ec0, "Sinking"},
    {0x1f24, "StartSwimming"}, {0x1f43, "Swimming"}, {0x1fe2, "SwimCalling"}, {0x2068, "SwimWaving"},
    {0x20c1, "SwimBoarding"},
};
const std::map<int, std::string> OBJECT_STATES = {
    {0x2379, "FlyerInit"}, {0x239f, "FlyerWait"}, {0x23b0, "FlyerScreech"}, {0x23d9, "FlyerWait2"},
    {0x23ea, "FlyerStart"}, {0x2493, "Flying"}, {0x252b, "FlyerHit"}, {0x255e, "FlyerFalling"},
    {0x25b1, "WalkerInit"}, {0x25c9, "Walking"}, {0x2667, "StartWatching"}, {0x2681, "Watching"},
    {0x272e, "StartCharging"}, {0x2748, "Charging"}, {0x2830, "StartRecovering"},
    {0x288f, "StartRecovering2"}, {0x2844, "Recovering"}, {0x28a3, "Recovering2"},
    {0x28ee, "WalkerStunned"}, {0x2914, "Stunned"}, {0x295b, "BlowerInit"}, {0x2973, "Blowing"},
    {0x2a53, "BlowerStunned"}, {0x2a76, "BlowerWait"}, {0x2a87, "TreeInit"}, {0x2ab5, "Tree"},
    {0x2b0c, "TreeCatch"}, {0x2b58, "TreeWait"}, {0x2b7e, "Inactive"}, {0x1c0e, "Gone"},
};
const std::map<int, std::string> BONUS_STATES = {{0x2b96, "Spawn"}, {0x2be4, "Falling"}, {0x2c97, "Landed"}, {0x2ca9, "Lying"}};
const std::map<int, std::string> OBJECT_KINDS = {{0x7630, "flyer"}, {0x766c, "walker"}, {0x76a8, "blower"}, {0x76e4, "tree"}};

std::string hex(int v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%04x", w16(v));
    return buf;
}

std::string named(const std::map<int, std::string>& names, int v, bool unknownAsHex) {
    auto it = names.find(v);
    if (it != names.end()) return it->second;
    char buf[16];
    std::snprintf(buf, sizeof buf, unknownAsHex ? "0x%04x" : "?0x%04x", v);
    return buf;
}

std::string format(Kind k, int word, int byte) {
    switch (k) {
        case Kind::S16: return std::to_string(s16(word));
        case Kind::U16: return std::to_string(word);
        case Kind::U8: return std::to_string(byte);
        case Kind::HEX: return hex(word);
        case Kind::HEX_OR_NONE: return word == 0xffff ? "none" : hex(word);
        case Kind::HEX_OR_NONE0: return word == 0 ? "none" : hex(word);
        case Kind::LEVEL_DONE: return (byte & 0x80) ? "1" : "0";
        case Kind::P_STATE: return named(PASSENGER_STATES, word, false);
        case Kind::O_STATE: return named(OBJECT_STATES, word, false);
        case Kind::B_STATE: return named(BONUS_STATES, word, false);
        case Kind::O_KIND: return named(OBJECT_KINDS, word, true);
    }
    return "";
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

std::optional<int> byName(const std::map<int, std::string>& names, const std::string& text) {
    for (const auto& [addr, name] : names) if (name == text) return addr;
    if (text.rfind("?0x", 0) == 0) return parseNumber(text.substr(3), 16);
    if (text.rfind("0x", 0) == 0) return parseNumber(text.substr(2), 16);
    return std::nullopt;
}

std::optional<int> parse(Kind k, const std::string& text) {
    switch (k) {
        case Kind::S16: case Kind::U16: case Kind::U8: return parseNumber(text, 10);
        case Kind::HEX: return text.rfind("0x", 0) == 0 ? parseNumber(text.substr(2), 16) : std::nullopt;
        case Kind::HEX_OR_NONE: if (text == "none") return 0xffff; return parse(Kind::HEX, text);
        case Kind::HEX_OR_NONE0: if (text == "none") return 0; return parse(Kind::HEX, text);
        case Kind::LEVEL_DONE: if (text == "1") return 0xfe; if (text == "0") return 0; return std::nullopt;
        case Kind::P_STATE: return byName(PASSENGER_STATES, text);
        case Kind::O_STATE: return byName(OBJECT_STATES, text);
        case Kind::B_STATE: return byName(BONUS_STATES, text);
        case Kind::O_KIND: return byName(OBJECT_KINDS, text);
    }
    return std::nullopt;
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

// key states of the projection: up, down, left, right, fire (the words at 278c / 2790 ... + bx)
constexpr int KEY_WORDS[5] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_FIRE};
constexpr char KEY_LETTERS[5] = {'U', 'D', 'L', 'R', 'F'};

}  // namespace

/** The raindrops as in StateProjection.rainChecksum (needs the private memory of Sim). */
struct Fields {
    static std::string rain(const Sim& s) {
        std::vector<uint8_t> bytes;
        for (int bx = 0; bx <= 0x180; bx += 2) {
            for (int v : {s.u(0x2e8b + bx), s.u(0x318f + bx)}) { bytes.push_back(static_cast<uint8_t>(v)); bytes.push_back(static_cast<uint8_t>(v >> 8)); }
        }
        char buf[16];
        std::snprintf(buf, sizeof buf, "%08x", crc32(bytes));
        return buf;
    }
};

bool Sim::known(int off, int length) const {
    for (int i = 0; i < length; i++) if (!known_[w16(off + i)]) return false;
    return true;
}

void Sim::reset() {
    std::copy(data_.dgroup.begin(), data_.dgroup.end(), mem_.begin());
    std::fill(known_.begin(), known_.end(), uint8_t{0});
    for (int si = 0; mem_[0x281c + si] != 0xff; si += 6) mem_[0x2821 + si] = 0;  // no key sequence in progress
    rng_ = {};
    rngKnown_ = rainKnown_ = false;
    lastScancode_ = 0;
    savedWaterRow_ = -1;
    problems.clear();
    game_ = Task();
    waiting_ = nullptr;
    result_ = 0;
}

void Sim::clear() {
    auto forget = [&](int off, int size) { for (int i = 0; i < size; i++) known_[w16(off + i)] = 0; };
    for (const Def& f : GAME) forget(f.off, sizeOf(f.kind));
    forget(0x261e, 4);
    for (const Group& g : GROUPS)
        for (int i = 0; i < g.slots; i++)
            for (size_t k = 0; k < g.count; k++) forget(g.defs[k].off + 2 * i, sizeOf(g.defs[k].kind));
    for (int w : KEY_WORDS) forget(w, 4);
    rngKnown_ = false;
}

int Sim::set(const std::string& field, const std::string& value) {
    auto dot = field.find('.');
    if (dot == std::string::npos) return 0;
    std::string group = field.substr(0, dot), rest = field.substr(dot + 1);
    if (group == "game") {
        if (rest == "score") {
            auto v = parseNumber(value, 10);
            if (!v) return -1;
            setD(0x261e, *v & 0xffff);
            setD(0x2620, (*v >> 16) & 0xffff);
            return 1;
        }
        if (rest == "rng") {
            if (value.size() != 16) return -1;
            for (int i = 0; i < 4; i++) {
                auto v = parseNumber(value.substr(4 * i, 4), 16);
                if (!v) return -1;
                rng_[3 - i] = *v;
            }
            rngKnown_ = true;
            return 1;
        }
        if (rest == "rain") {
            // the drops are not in the replay: the core keeps its own while they match the checksum
            if (value == "none" || !rainKnown_ || Fields::rain(*this) != value) rainKnown_ = false;
            return 1;
        }
        for (const Def& f : GAME) {
            if (rest != f.name) continue;
            auto v = parse(f.kind, value);
            if (!v) return -1;
            if (sizeOf(f.kind) == 1) setD8(f.off, *v); else setD(f.off, *v);
            return 1;
        }
        return 0;
    }
    auto dot2 = rest.find('.');
    if (dot2 == std::string::npos) return 0;
    auto index = parseNumber(rest.substr(0, dot2), 10);
    std::string name = rest.substr(dot2 + 1);
    for (const Group& g : GROUPS) {
        if (group != g.name) continue;
        if (!index || *index < 0 || *index >= g.slots) return 0;
        int bx = 2 * *index;
        if (group == "copter" && name == "keys") {
            for (int k = 0; k < 5; k++) setD(KEY_WORDS[k] + bx, value.find(KEY_LETTERS[k]) != std::string::npos ? 0xff : 0);
            return 1;
        }
        for (size_t k = 0; k < g.count; k++) {
            if (name != g.defs[k].name) continue;
            auto v = parse(g.defs[k].kind, value);
            if (!v) return -1;
            setD(g.defs[k].off + bx, *v);
            return 1;
        }
        return 0;
    }
    return 0;
}

std::vector<std::pair<std::string, std::string>> Sim::fields() const {
    std::vector<std::pair<std::string, std::string>> out;
    for (const Def& f : GAME) {
        if (known(f.off, sizeOf(f.kind))) out.emplace_back(std::string("game.") + f.name, format(f.kind, u(f.off), d8(f.off)));
    }
    if (known(0x261e, 4)) out.emplace_back("game.score", std::to_string((static_cast<int64_t>(u(0x2620)) << 16) | u(0x261e)));
    if (rngKnown_) {
        char buf[20];
        std::snprintf(buf, sizeof buf, "%04x%04x%04x%04x", rng_[3], rng_[2], rng_[1], rng_[0]);
        out.emplace_back("game.rng", buf);
    }
    if (known(V_WIND, 1)) {
        if (d8(V_WIND) == 0) out.emplace_back("game.rain", "none");
        else if (rainKnown_) out.emplace_back("game.rain", Fields::rain(*this));
    }
    int copters = known(PLAYERS, 2) ? std::clamp(u(PLAYERS), 1, 2) : 2;
    for (const Group& g : GROUPS) {
        std::string group = g.name;
        for (int i = 0; i < g.slots; i++) {
            int bx = 2 * i;
            // the lists end like in the projection; bonus slots are listed while used
            if (group == "copter" && i >= copters) break;
            if (group == "pad" && (!known(PAD_LEFT + bx, 2) || d(PAD_LEFT + bx) < 0)) break;
            if (group == "passenger" && (!known(0x29ad + bx, 2) || u(0x29ad + bx) == 0xffff)) break;
            if (group == "object" && (!known(0x2cad + bx, 2) || u(0x2cad + bx) == 0xffff)) break;
            if (group == "bonus" && (!known(0x2d9b + bx, 2) || u(0x2d9b + bx) == 0xffff)) continue;
            std::string prefix = group + "." + std::to_string(i) + ".";
            for (size_t k = 0; k < g.count; k++) {
                const Def& f = g.defs[k];
                if (known(f.off + bx, 2)) out.emplace_back(prefix + f.name, format(f.kind, u(f.off + bx), d8(f.off + bx)));
            }
            if (group == "copter") {
                bool all = true;
                std::string keys;
                for (int k = 0; k < 5; k++) {
                    all = all && known(KEY_WORDS[k] + bx, 2);
                    if (u(KEY_WORDS[k] + bx) != 0) keys += KEY_LETTERS[k];
                }
                if (all) out.emplace_back(prefix + "keys", keys.empty() ? "-" : keys);
            }
        }
    }
    return out;
}

/**
 * What the replay leaves out but the logic reads and that follows from other fields (StateProjection.NOT_PROJECTED):
 * players * 2 (2636, set with 2634 by F4 in the menu), the level record (from the level number; passengers left,
 * wind and water are fields), the pickup / target pads * 2 (set together with them).
 */
void Sim::derive() {
    if (known(PLAYERS, 2)) setD(PLAYERS2, u(PLAYERS) << 1);
    if (known(0x261c, 2) && known(PLAYERS, 2)) {
        int rec = levelRecord();
        for (int i = 0; i < 0x1a; i++) {
            int off = 0x28e9 + i;
            if (off == 0x28f1 || off == V_WIND || off == 0x28fe || off == 0x28ff) continue;
            mem_[off] = mem_[w16(rec + i)];
            known_[off] = 1;
        }
    }
    for (int bx = 0; bx < 0x20 && known(0x29ad + bx, 2) && u(0x29ad + bx) != 0xffff; bx += 2) {
        setD(0x2a8d + bx, u(0x2a4d + bx) << 1);
        setD(0x2aad + bx, u(0x2a6d + bx) << 1);
    }
}

/**
 * Before a single transition from a recorded state: the ends of the lists (the replay lists only the entries
 * before them) and the free bonus slots, then what follows from the fields.
 */
void Sim::prepare() {
    auto terminate = [&](int base, int slots, auto ended) {
        for (int bx = 0; bx < 2 * slots; bx += 2) {
            if (!known(base + bx, 2)) { setD(base + bx, 0xffff); return; }
            if (ended(base + bx)) return;
        }
    };
    terminate(PAD_LEFT, 10, [&](int off) { return d(off) < 0; });
    terminate(0x29ad, 16, [&](int off) { return u(off) == 0xffff; });
    terminate(0x2cad, 5, [&](int off) { return u(off) == 0xffff; });
    for (int bx = 0; bx < 0x18; bx += 2) if (!known(0x2d9b + bx, 2)) setD(0x2d9b + bx, 0xffff);
    derive();
}

}  // namespace ugh
