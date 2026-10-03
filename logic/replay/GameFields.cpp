#include "replay/GameFields.hpp"

#include <cstdio>

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

}  // namespace

void GameFields::write(const game::Game& game, Fields& f) {
    const world::Session& session = game.session();
    f["game.phase"] = phaseName(game.phase());
    f["game.level"] = std::to_string(session.levelNumber());
    f["game.players"] = std::to_string(session.players());
    f["game.difficulty"] = difficultyName(session.difficulty());
    f["game.lives"] = std::to_string(session.lives());
    f["game.multiplier"] = std::to_string(session.multiplier());
    f["game.score"] = std::to_string(session.score());
    const world::RandomNumbers::Words& words = session.random().words();
    char rng[20];
    std::snprintf(rng, sizeof rng, "%04x%04x%04x%04x", words[3], words[2], words[1], words[0]);
    f["game.rng"] = rng;
    f["game.rainFloor"] = std::to_string(game.level().rain().floorRow());
}

}  // namespace ugh::replay
