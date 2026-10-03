#include "replay/GameFields.hpp"

#include <cstdio>
#include <vector>

#include "data/GameData.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

namespace {

constexpr int WINDS = 3;   // none, to the left, to the right

/** "0" or "1". */
class FlagField : public Field {
public:
    explicit FlagField(bool& value) : value_(value) {}

    std::optional<std::string> text() const override { return value_ ? "1" : "0"; }

    bool read(const std::string& text) const override {
        if (text != "0" && text != "1") return false;
        value_ = text == "1";
        return true;
    }

    void fill(int pattern) const override { value_ = pattern != 0; }

private:
    bool& value_;
};

/** The row where the raindrops start again: the original keeps its byte offset in the VGA page (96 per row). */
class RainFloorField : public Field {
public:
    explicit RainFloorField(uint8_t& row) : row_(row) {}

    std::optional<std::string> text() const override { return std::to_string(row_ * BYTES_PER_ROW); }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseNumber(text);
        if (!v || *v % BYTES_PER_ROW != 0 || *v / BYTES_PER_ROW > 0xff) return false;
        row_ = static_cast<uint8_t>(*v / BYTES_PER_ROW);
        return true;
    }

    void fill(int pattern) const override { row_ = static_cast<uint8_t>(pattern); }

private:
    static constexpr int BYTES_PER_ROW = 96;
    uint8_t& row_;
};

/** The score: 32 bits; the pattern fills both words. */
class ScoreField : public Field {
public:
    explicit ScoreField(uint32_t& score) : score_(score) {}

    std::optional<std::string> text() const override { return std::to_string(score_); }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseNumber(text);
        if (v) score_ = static_cast<uint32_t>(*v);
        return v.has_value();
    }

    void fill(int pattern) const override { score_ = static_cast<uint16_t>(pattern) * 0x10001u; }

private:
    uint32_t& score_;
};

/** The four words of the random generator, the last first, in hex. */
class RandomField : public Field {
public:
    explicit RandomField(core::Random::Snapshot& words) : words_(words) {}

    std::optional<std::string> text() const override {
        char buf[20];
        std::snprintf(buf, sizeof buf, "%04x%04x%04x%04x", words_[3], words_[2], words_[1], words_[0]);
        return buf;
    }

    bool read(const std::string& text) const override {
        if (text.size() != 16) return false;
        for (int i = 0; i < 4; i++) {
            std::optional<int> v = parseNumber(text.substr(4 * i, 4), 16);
            if (!v) return false;
            words_[3 - i] = static_cast<uint16_t>(*v);
        }
        return true;
    }

    void fill(int pattern) const override { words_.fill(static_cast<uint16_t>(pattern)); }

private:
    core::Random::Snapshot& words_;
};

/** CRC-32 (zlib) as in StateProjection.rainChecksum. */
uint32_t crc32(const std::vector<uint8_t>& bytes) {
    uint32_t crc = 0xffffffffu;
    for (uint8_t b : bytes) {
        crc ^= b;
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
    }
    return ~crc;
}

/**
 * The raindrops: "none" without wind, else a checksum of the drops as the original keeps them (a byte offset in
 * the VGA page and a plane, 2e8b / 318f). The core keeps its own drops while they match the checksum.
 */
class RainField : public Field {
public:
    RainField(model::Rain::Snapshot& rain, const uint8_t& wind, int unknownPattern)
        : rain_(rain), wind_(wind), unknownPattern_(unknownPattern) {}

    std::optional<std::string> text() const override { return wind_ == 0 ? "none" : checksum(); }

    bool read(const std::string& text) const override {
        if (text != "none" && text != checksum()) rain_.drops.fill(unknownPattern_);
        return true;
    }

    void fill(int) const override {}

private:
    model::Rain::Snapshot& rain_;
    const uint8_t& wind_;
    int unknownPattern_;

    std::string checksum() const {
        std::vector<uint8_t> bytes;
        for (int32_t drop : rain_.drops) {
            for (int v : {drop >> 2, drop & 3}) {
                bytes.push_back(static_cast<uint8_t>(v));
                bytes.push_back(static_cast<uint8_t>(v >> 8));
            }
        }
        char buf[16];
        std::snprintf(buf, sizeof buf, "%08x", crc32(bytes));
        return buf;
    }
};

}  // namespace

GameFields::State GameFields::State::of(const model::Level& level) {
    return {level.session().snapshot(), level.session().random().snapshot(), level.snapshot(),
            level.water().snapshot(), level.rain().snapshot()};
}

void GameFields::State::restoreTo(model::Level& target) const {
    target.session().restore(session);
    target.session().random().restore(random);
    target.restore(level);
    target.water().restore(water);
    target.rain().restore(rain);
}

void GameFields::visit(FieldVisitor& v, State& s, int unknownPattern) {
    v.unsignedWord("level", s.session.levelNumber);
    v.unsignedWord("players", s.session.players);
    v.index("difficulty", s.session.difficulty, data::GameData::DIFFICULTIES, 1);   // a value of the rules for each
    v.byte("lives", s.session.lives);
    v.byte("multiplier", s.session.multiplier);
    core::Word energy = s.level.energy.value();
    v.signedWord("energy", energy);
    s.level.energy = model::Energy(energy);
    core::Word fade = s.level.fade.position(), fadeStep = s.level.fade.step();
    v.signedWord("fade", fade);
    v.signedWord("fadeStep", fadeStep);
    s.level.fade = model::Fade(fade, fadeStep);
    v.index("wind", s.level.wind, WINDS, 2);
    v.signedWord("waterRow", s.water.row);
    v.byte("passengersLeft", s.level.passengersLeft);
    v.signedWord("waterYf", s.water.level);
    v.byte("waterHold", s.water.hold);
    v.byte("waterToggle", s.water.toggle);
    v.signedWord("waterAnim", s.water.surfaceFrame);
    v.byte("waterAnimDelay", s.water.surfaceDelay);
    v.visit("levelDone", FlagField(s.level.done));
    v.visit("rainFloor", RainFloorField(s.rain.floorRow));
    v.visit("score", ScoreField(s.session.score));
    v.visit("rng", RandomField(s.random));
    v.visit("rain", RainField(s.rain, s.level.wind, unknownPattern));
}

}  // namespace ugh::replay
