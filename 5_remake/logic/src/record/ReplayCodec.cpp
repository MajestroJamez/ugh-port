#include "record/ReplayCodec.hpp"

#include "record/ByteReader.hpp"
#include "record/ByteWriter.hpp"
#include "record/Crc32.hpp"
#include "world/session/Lives.hpp"

namespace ugh::record {

namespace {

// an input is one number: the steps since the input before (its gap) and a code of 6 bits
constexpr int CODE_BITS = 6;
constexpr int PILOT_CODES = 2 * 5 * 2;        // player, key, pressed
constexpr int WITH_OTHER = PILOT_CODES;       // + a pilot's code: that key, then a menu key Other between the same steps
constexpr int MENU_CODES = 2 * PILOT_CODES;   // + the menu key (Escape, Pause, Other)
constexpr int CODES = MENU_CODES + 3;
constexpr int MAX_LEVEL = 999, MAX_MULTIPLIER = 99, MAX_EFFORT = 65536;

int pilotCode(const Input& in) { return in.player * 10 + static_cast<int>(in.key) * 2 + (in.pressed ? 1 : 0); }

/** At most MAX_TEXT bytes (not cutting a character of UTF-8 apart), nothing below a space. */
std::string clean(const std::string& text) {
    std::string kept;
    for (char c : text)
        if (static_cast<uint8_t>(c) >= 0x20 && c != 0x7F) kept.push_back(c);
    if (kept.size() <= Recording::MAX_TEXT) return kept;
    size_t size = Recording::MAX_TEXT;
    while (size > 0 && (static_cast<uint8_t>(kept[size]) & 0xC0) == 0x80) size--;
    return kept.substr(0, size);
}

void writeInputs(ByteWriter& w, const std::vector<Input>& inputs) {
    std::vector<uint64_t> tokens;
    int last = 0;
    for (size_t i = 0; i < inputs.size(); i++) {
        const Input& in = inputs[i];
        int code = in.kind == Input::Kind::MenuKey ? MENU_CODES + static_cast<int>(in.menuKey) : pilotCode(in);
        if (in.kind == Input::Kind::PilotKey && i + 1 < inputs.size()) {
            const Input& next = inputs[i + 1];
            if (next.kind == Input::Kind::MenuKey && next.menuKey == input::MenuKey::Other && next.after == in.after) {
                code += WITH_OTHER;   // (a frontend tells the game loop of every key event)
                i++;
            }
        }
        tokens.push_back(static_cast<uint64_t>(in.after - last) << CODE_BITS | static_cast<uint64_t>(code));
        last = in.after;
    }
    w.number(tokens.size());
    for (uint64_t token : tokens) w.number(token);
}

bool readInputs(ByteReader& r, int steps, std::vector<Input>& inputs) {
    int64_t count = r.number(0, ReplayCodec::MAX_INPUTS);
    int64_t after = 0;
    for (int64_t i = 0; i < count && r.ok(); i++) {
        uint64_t token = r.number();
        uint64_t gap = token >> CODE_BITS;
        int code = static_cast<int>(token & ((1 << CODE_BITS) - 1));
        if (gap >= static_cast<uint64_t>(steps) || after + static_cast<int64_t>(gap) >= steps || code >= CODES) return false;
        after += static_cast<int64_t>(gap);
        int at = static_cast<int>(after);
        if (code >= MENU_CODES) {
            inputs.push_back(Input::menu(at, static_cast<input::MenuKey>(code - MENU_CODES)));
            continue;
        }
        int pilot = code % WITH_OTHER;
        inputs.push_back(Input::pilot(at, pilot / 10, static_cast<input::PlayerKey>(pilot % 10 / 2), pilot % 2 == 1));
        if (code >= WITH_OTHER) inputs.push_back(Input::menu(at, input::MenuKey::Other));
    }
    return r.ok();
}

}  // namespace

std::vector<uint8_t> ReplayCodec::write(const Recording& recording) {
    ByteWriter w;
    for (char c : MAGIC) w.byte(static_cast<uint8_t>(c));
    w.byte(FORMAT);
    w.number(static_cast<uint64_t>(recording.logicVersion));
    w.word32(recording.dataHash);
    const game::AttemptStart& s = recording.start;
    w.byte(static_cast<uint8_t>(s.players));
    w.byte(static_cast<uint8_t>(s.difficulty));
    w.number(static_cast<uint64_t>(s.level));
    w.number(static_cast<uint64_t>(s.lives));
    w.number(s.points);
    w.number(static_cast<uint64_t>(s.multiplier));
    for (uint16_t word : s.random) w.word16(word);
    w.byte(static_cast<uint8_t>(s.rainFloorRow));
    for (int player = 0; player < s.players && player < 2; player++) w.signedNumber(s.effort[player]);
    w.byte(static_cast<uint8_t>(s.lastMenuKey));
    w.number(static_cast<uint64_t>(recording.steps));
    w.number(static_cast<uint64_t>(recording.playSteps));
    w.number(static_cast<uint64_t>(recording.attempts));
    w.number(recording.points);
    w.byte(recording.done ? 1 : 0);
    w.number(recording.date > 0 ? static_cast<uint64_t>(recording.date) : 0);
    w.text(clean(recording.password));
    size_t names = recording.names.size() < Recording::MAX_NAMES ? recording.names.size() : Recording::MAX_NAMES;
    w.byte(static_cast<uint8_t>(names));
    for (size_t i = 0; i < names; i++) w.text(clean(recording.names[i]));
    writeInputs(w, recording.inputs);
    w.word32(Crc32::of(w.bytes()));
    return w.bytes();
}

std::optional<Recording> ReplayCodec::read(std::span<const uint8_t> bytes, std::string& error) {
    const size_t head = MAGIC.size() + 1;
    if (bytes.size() < head || std::string_view(reinterpret_cast<const char*>(bytes.data()), MAGIC.size()) != MAGIC) {
        error = NOT_A_REPLAY;
        return std::nullopt;
    }
    const int format = bytes[MAGIC.size()];
    error = format > FORMAT ? NEWER : format == 0 || bytes.size() < head + 4 ? DAMAGED : "";
    if (!error.empty()) return std::nullopt;
    const size_t body = bytes.size() - 4;
    ByteReader crc(bytes.subspan(body));
    if (crc.word32() != Crc32::of(bytes.first(body))) {
        error = DAMAGED_CHECKSUM;
        return std::nullopt;
    }
    ByteReader r(bytes.subspan(head, body - head));
    Recording rec;
    rec.logicVersion = static_cast<int>(r.number(0, 0xFFFF));
    rec.dataHash = r.word32();
    game::AttemptStart& s = rec.start;
    s.players = r.byte();
    const int difficulty = r.byte();
    s.difficulty = static_cast<data::Difficulty>(difficulty);
    s.level = static_cast<int>(r.number(0, MAX_LEVEL));
    s.lives = static_cast<int>(r.number(0, world::session::Lives::MAX));
    s.points = static_cast<uint32_t>(r.number(0, UINT32_MAX));
    s.multiplier = static_cast<int>(r.number(1, MAX_MULTIPLIER));
    for (uint16_t& word : s.random) word = r.word16();
    s.rainFloorRow = r.byte();
    const bool players = s.players == 1 || s.players == 2;
    for (int player = 0; players && player < s.players; player++)
        s.effort[player] = static_cast<int>(r.signedNumber(-MAX_EFFORT, MAX_EFFORT));
    const int menuKey = r.byte();
    s.lastMenuKey = static_cast<input::MenuKey>(menuKey);
    rec.steps = static_cast<int>(r.number(1, MAX_STEPS));
    rec.playSteps = static_cast<int>(r.number(0, rec.steps));
    rec.attempts = static_cast<int>(r.number(1, rec.steps));
    rec.points = static_cast<uint32_t>(r.number(0, UINT32_MAX));
    const int done = r.byte();
    rec.done = done == 1;
    rec.date = r.number(0, INT64_MAX);
    rec.password = r.text(Recording::MAX_TEXT);
    const int names = r.byte();
    for (int i = 0; i < names && i < Recording::MAX_NAMES; i++) rec.names.push_back(r.text(Recording::MAX_TEXT));
    const bool valid = players && difficulty <= 2 && menuKey <= 2 && done <= 1 && names <= Recording::MAX_NAMES;
    if (!valid || !readInputs(r, rec.steps, rec.inputs) || !r.atEnd()) {
        error = DAMAGED;
        return std::nullopt;
    }
    error.clear();
    return rec;
}

}  // namespace ugh::record
