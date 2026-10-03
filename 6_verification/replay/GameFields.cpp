#include "replay/GameFields.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

namespace ugh::replay {

namespace {

const char* phaseName(game::GamePhase phase) {
    switch (phase) {
        case game::GamePhase::Start: return "start";
        case game::GamePhase::BetweenLevels: return "betweenLevels";
        case game::GamePhase::Caption: return "caption";
        case game::GamePhase::Setup: return "setup";
        case game::GamePhase::Play: return "play";
    }
    return "";
}

const char* difficultyName(data::Difficulty difficulty) {
    switch (difficulty) {
        case data::Difficulty::Easy: return "easy";
        case data::Difficulty::Medium: return "medium";
        case data::Difficulty::Hard: return "hard";
    }
    return "";
}

const char* windName(data::levels::Wind wind) {
    switch (wind) {
        case data::levels::Wind::None: return "none";
        case data::levels::Wind::Left: return "left";
        case data::levels::Wind::Right: return "right";
    }
    return "";
}

/** CRC-32 (zlib). */
uint32_t crc32(const std::vector<uint8_t>& bytes) {
    uint32_t crc = 0xffffffffu;
    for (uint8_t b : bytes) {
        crc ^= b;
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
    }
    return ~crc;
}

/** The raindrops 0 .. 192 as little-endian int16 pairs x, y, CRC-32 in 8 hex digits. */
std::string rainChecksum(const world::scenery::Rain& rain) {
    std::vector<uint8_t> bytes;
    for (const world::scenery::Raindrop& drop : rain.drops()) {
        for (int v : {drop.x, drop.y}) {
            bytes.push_back(static_cast<uint8_t>(v));
            bytes.push_back(static_cast<uint8_t>(v >> 8));
        }
    }
    char buf[16];
    std::snprintf(buf, sizeof buf, "%08x", crc32(bytes));
    return buf;
}

}  // namespace

void GameFields::write(const game::Game& game, Fields& f) {
    const world::session::Session& session = game.session();
    f["game.phase"] = phaseName(game.phase());
    f["game.level"] = std::to_string(session.levelNumber());
    f["game.players"] = std::to_string(session.players());
    f["game.difficulty"] = difficultyName(session.difficulty());
    f["game.lives"] = std::to_string(session.lives().count());
    f["game.multiplier"] = std::to_string(session.score().multiplier());
    f["game.score"] = std::to_string(session.score().points());
    const world::session::RandomNumbers::Words& words = session.random().words();
    char rng[20];
    std::snprintf(rng, sizeof rng, "%04x%04x%04x%04x", words[3], words[2], words[1], words[0]);
    f["game.rng"] = rng;
    const world::Level& level = game.level();
    f["game.rainFloor"] = std::to_string(level.rain().floorRow());
    if (!game.levelLoaded()) return;
    f["game.energy"] = std::to_string(level.energy().value());
    f["game.fade"] = std::to_string(level.fade().position());
    f["game.fadeDirection"] = level.fade().fadingOut() ? "out" : "in";
    f["game.levelDone"] = level.delivery().done() ? "1" : "0";
    f["game.wind"] = windName(level.wind());
    f["game.passengersLeft"] = std::to_string(level.delivery().left());
    const world::scenery::Water& water = level.water();
    f["game.water.level"] = std::to_string(water.level().raw());
    f["game.water.resting"] = water.resting() ? "1" : "0";
    f["game.water.evenFrame"] = std::to_string(water.evenFrame());
    f["game.water.surfaceFrame"] = std::to_string(water.surfaceFrame());
    f["game.water.surfaceDelay"] = std::to_string(water.surfaceDelay());
    f["game.rain"] = level.windy() ? rainChecksum(level.rain()) : "none";
}

}  // namespace ugh::replay
