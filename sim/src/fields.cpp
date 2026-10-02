// The state as the named fields of the golden replays - the C++ side of
// verify/src/test/kotlin/ugh/verify/replay/StateProjection.kt (same names, values and formats).
#include "sim.hpp"

#include <cstdio>
#include <algorithm>
#include <functional>

namespace ugh {

namespace {

enum class Fmt { S16, U16, U8, HEX, HEX_OR_NONE0 };

std::string format(Fmt f, int raw) {
    char buf[16];
    switch (f) {
        case Fmt::S16: std::snprintf(buf, sizeof buf, "%d", s16(raw)); break;
        case Fmt::U16: std::snprintf(buf, sizeof buf, "%d", w16(raw)); break;
        case Fmt::U8: std::snprintf(buf, sizeof buf, "%d", b8(raw)); break;
        case Fmt::HEX: std::snprintf(buf, sizeof buf, "0x%04x", w16(raw)); break;
        case Fmt::HEX_OR_NONE0: if (w16(raw) == 0) return "none"; std::snprintf(buf, sizeof buf, "0x%04x", w16(raw)); break;
    }
    return buf;
}

bool parse(Fmt f, const std::string& text, int& raw) {
    if ((f == Fmt::HEX_OR_NONE0) && text == "none") { raw = 0; return true; }
    try {
        size_t used = 0;
        long v = (f == Fmt::HEX || f == Fmt::HEX_OR_NONE0) ? std::stol(text, &used, 16) : std::stol(text, &used, 10);
        if (used != text.size()) return false;
        raw = f == Fmt::U8 ? b8(static_cast<int>(v)) : w16(static_cast<int>(v));
        return true;
    } catch (...) {
        return false;
    }
}

struct Field {
    std::string name;
    Fmt fmt;
    std::function<int&(State&)> ref;
};

/** The plain fields of StateProjection; the special ones (score, level done, rng, rain, keys) are handled apart. */
std::vector<Field> plainFields() {
    std::vector<Field> f = {
        {"game.level", Fmt::U16, [](State& s) -> int& { return s.level; }},
        {"game.players", Fmt::U16, [](State& s) -> int& { return s.players; }},
        {"game.difficulty", Fmt::U16, [](State& s) -> int& { return s.difficulty; }},
        {"game.lives", Fmt::U8, [](State& s) -> int& { return s.lives; }},
        {"game.multiplier", Fmt::U8, [](State& s) -> int& { return s.multiplier; }},
        {"game.energy", Fmt::S16, [](State& s) -> int& { return s.energy; }},
        {"game.fade", Fmt::S16, [](State& s) -> int& { return s.fade; }},
        {"game.fadeStep", Fmt::S16, [](State& s) -> int& { return s.fadeStep; }},
        {"game.wind", Fmt::U8, [](State& s) -> int& { return s.wind; }},
        {"game.waterRow", Fmt::S16, [](State& s) -> int& { return s.waterRow; }},
        {"game.rainFloor", Fmt::U16, [](State& s) -> int& { return s.rainFloor; }},
    };
    std::vector<Field> level = {
        {"game.passengersLeft", Fmt::U8, [](State& s) -> int& { return s.passengersLeft; }},
        {"game.waterYf", Fmt::S16, [](State& s) -> int& { return s.waterYf; }},
        {"game.waterHold", Fmt::U8, [](State& s) -> int& { return s.waterHold; }},
        {"game.waterToggle", Fmt::U8, [](State& s) -> int& { return s.waterToggle; }},
        {"game.waterAnim", Fmt::S16, [](State& s) -> int& { return s.waterAnim; }},
        {"game.waterAnimDelay", Fmt::U8, [](State& s) -> int& { return s.waterAnimDelay; }},
    };
    f.insert(f.end(), level.begin(), level.end());
    for (int p = 0; p < 2; p++) {
        std::string c = "copter." + std::to_string(p) + ".";
        auto cp = [p](auto member) { return [p, member](State& s) -> int& { return s.copters[p].*member; }; };
        std::vector<Field> copter = {
            {c + "xf", Fmt::S16, cp(&Copter::xf)}, {c + "yf", Fmt::S16, cp(&Copter::yf)},
            {c + "vx", Fmt::S16, cp(&Copter::vx)}, {c + "vy", Fmt::S16, cp(&Copter::vy)},
            {c + "landedPad", Fmt::S16, cp(&Copter::landed)}, {c + "effort", Fmt::S16, cp(&Copter::effort)},
            {c + "impact", Fmt::S16, cp(&Copter::impact)}, {c + "x", Fmt::S16, cp(&Copter::x)},
            {c + "y", Fmt::S16, cp(&Copter::y)}, {c + "sprite", Fmt::HEX, cp(&Copter::sprite)},
            {c + "animCounter", Fmt::S16, cp(&Copter::animCounter)},
            {c + "carrying", Fmt::HEX_OR_NONE0, cp(&Copter::carrying)}, {c + "targetPad", Fmt::S16, cp(&Copter::targetPad)},
            {c + "fare", Fmt::S16, cp(&Copter::fare)}, {c + "fareMin", Fmt::S16, cp(&Copter::fareMin)},
        };
        f.insert(f.end(), copter.begin(), copter.end());
    }
    for (int i = 0; i < PADS; i++) {
        std::string c = "pad." + std::to_string(i) + ".";
        auto pp = [i](auto member) { return [i, member](State& s) -> int& { return s.pads[i].*member; }; };
        std::vector<Field> pad = {
            {c + "left", Fmt::S16, pp(&Pad::left)}, {c + "right", Fmt::S16, pp(&Pad::right)},
            {c + "y", Fmt::S16, pp(&Pad::y)}, {c + "number", Fmt::S16, pp(&Pad::number)},
            {c + "waiting", Fmt::S16, pp(&Pad::waiting)}, {c + "doorX", Fmt::S16, pp(&Pad::doorX)},
            {c + "waitX", Fmt::S16, pp(&Pad::waitX)}, {c + "standX", Fmt::S16, pp(&Pad::standX)},
        };
        f.insert(f.end(), pad.begin(), pad.end());
    }
    return f;
}

const std::vector<Field>& allFields() {
    static const std::vector<Field> f = plainFields();
    return f;
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

std::string rainChecksum(const State& s) {
    std::vector<uint8_t> bytes;
    for (int i = 0; i < RAINDROPS; i++) {
        for (int v : {s.rainOffset[i], s.rainPlane[i]}) { bytes.push_back(static_cast<uint8_t>(v)); bytes.push_back(static_cast<uint8_t>(v >> 8)); }
    }
    char buf[16];
    std::snprintf(buf, sizeof buf, "%08x", crc32(bytes));
    return buf;
}

// key slots of the projection: up, down, left, right (278c / 2790 / 2794 / 2798 + bx), fire (279c + bx)
constexpr int KEY_SLOTS[5] = {0x00, 0x04, 0x08, 0x0c, 0x10};
constexpr char KEY_LETTERS[5] = {'U', 'D', 'L', 'R', 'F'};

}  // namespace

void Sim::reset() {
    State fresh;
    fresh.keyMatch.assign(data_.dgroup.begin() + 0x281c, data_.dgroup.begin() + 0x2a1c);
    for (size_t i = 5; i < fresh.keyMatch.size(); i += 6) fresh.keyMatch[i] = 0;  // no sequence in progress
    state = fresh;
}

void Sim::clear() {
    // what the replay does not hold survives: the raindrops (checked against game.rain), the keyboard handler
    State kept = state;
    reset();
    state.rainKnown = kept.rainKnown;
    state.rainOffset = kept.rainOffset;
    state.rainPlane = kept.rainPlane;
    state.lastScancode = kept.lastScancode;
    state.keyPosition = kept.keyPosition;
    state.keyMatch = kept.keyMatch;
    state.savedWaterRow = kept.savedWaterRow;
}

int Sim::set(const std::string& field, const std::string& value) {
    State& s = state;
    if (field == "game.score") {
        try { long long v = std::stoll(value); s.scoreLo = static_cast<int>(v & 0xffff); s.scoreHi = static_cast<int>((v >> 16) & 0xffff); return 1; }
        catch (...) { return -1; }
    }
    if (field == "game.levelDone") { s.levelDone = value == "1" ? 0xfe : 0; return value == "0" || value == "1" ? 1 : -1; }
    if (field == "game.rng") {
        if (value.size() != 16) return -1;
        try { for (int i = 0; i < 4; i++) s.rng[3 - i] = static_cast<int>(std::stol(value.substr(4 * i, 4), nullptr, 16)); return 1; }
        catch (...) { return -1; }
    }
    if (field == "game.rain") {
        // the drops themselves are not in the replay: the core keeps its own if they still match
        if (value == "none" || !s.rainKnown || rainChecksum(s) != value) s.rainKnown = false;
        return 1;
    }
    if (field.size() == 13 && field.rfind("copter.", 0) == 0 && field.substr(8) == ".keys") {
        int p = field[7] - '0';
        if (p < 0 || p > 1) return 0;
        for (int k = 0; k < 5; k++) s.keys[KEY_SLOTS[k] + 2 * p] = value.find(KEY_LETTERS[k]) != std::string::npos ? 0xff : 0;
        s.copters[p].keysKnown = true;
        return 1;
    }
    for (const Field& f : allFields()) {
        if (f.name != field) continue;
        int raw;
        if (!parse(f.fmt, value, raw)) return -1;
        f.ref(s) = raw;
        return 1;
    }
    return 0;
}

std::vector<std::pair<std::string, std::string>> Sim::fields() const {
    State& s = const_cast<State&>(state);
    std::vector<std::pair<std::string, std::string>> out;
    auto known = [](int v) { return v != UNKNOWN; };
    auto prefixed = [](const std::string& name, const std::string& prefix) { return name.compare(0, prefix.size(), prefix) == 0; };
    int copters = known(s.players) ? std::clamp(s.players, 1, 2) : 2;
    int pads = 0;
    while (pads < PADS && known(s.pads[pads].left) && s16(s.pads[pads].left) >= 0) pads++;
    for (const Field& f : allFields()) {
        if (!known(f.ref(s))) continue;
        if (prefixed(f.name, "copter.") && f.name[7] - '0' >= copters) continue;
        if (prefixed(f.name, "pad.") && std::stoi(f.name.substr(4)) >= pads) continue;
        out.emplace_back(f.name, format(f.fmt, f.ref(s)));
    }
    if (known(s.scoreLo) && known(s.scoreHi)) out.emplace_back("game.score", std::to_string((static_cast<long long>(s.scoreHi) << 16) | s.scoreLo));
    if (known(s.levelDone)) out.emplace_back("game.levelDone", (s.levelDone & 0x80) ? "1" : "0");
    if (known(s.rng[0]) && known(s.rng[1]) && known(s.rng[2]) && known(s.rng[3])) {
        char buf[20];
        std::snprintf(buf, sizeof buf, "%04x%04x%04x%04x", s.rng[3], s.rng[2], s.rng[1], s.rng[0]);
        out.emplace_back("game.rng", buf);
    }
    if (known(s.wind)) {
        if (b8(s.wind) == 0) out.emplace_back("game.rain", "none");
        else if (s.rainKnown) out.emplace_back("game.rain", rainChecksum(s));
    }
    for (int p = 0; p < copters; p++) {
        if (!s.copters[p].keysKnown) continue;
        std::string k;
        for (int i = 0; i < 5; i++) if (key16(KEY_AREA + KEY_SLOTS[i] + 2 * p) != 0) k += KEY_LETTERS[i];
        out.emplace_back("copter." + std::to_string(p) + ".keys", k.empty() ? "-" : k);
    }
    return out;
}

int Sim::key16(int off) const {
    int i = off - KEY_AREA;
    return state.keys[i] | (i + 1 < static_cast<int>(state.keys.size()) ? state.keys[i + 1] << 8 : 0);
}

}  // namespace ugh
