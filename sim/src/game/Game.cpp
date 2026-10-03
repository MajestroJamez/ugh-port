#include "game/Game.hpp"

#include "game/LevelLoader.hpp"
#include "ugh_sim.h"

namespace ugh::game {

Game::Game(const data::GameData& data)
    : session_(data),
      level_(data, session_, events_, diagnostics_),
      keyboard_(data.keys()),
      frame_(data, level_, keyboard_),
      flow_(*this) {}

void Game::reset() {
    session_.restore(model::GameSession::Snapshot{});
    session_.random().restore(core::Random::Snapshot{});
    level_.reset();
    keyboard_.reset();
    diagnostics_.clear();
    flow_.reset();
}

int Game::levelEnd() {
    if (level_.done()) {
        if (!session_.nextLevel()) return UGH_SIM_ALL_LEVELS_DONE;
    } else {
        if (!session_.loseLife()) return UGH_SIM_GAME_OVER;
    }
    return UGH_SIM_CONTINUE;
}

void Game::levelStart() {
    LevelLoader::startAttempt(level_);
    level_.water().setRow(CAPTION_WATER_ROW);
}

void Game::playFrame() {
    level_.fade().advance();
    frame_.run();
}

}  // namespace ugh::game
