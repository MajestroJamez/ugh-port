#include "game/Game.hpp"

namespace ugh::game {

Game::Game(const data::GameData& data) : data_(data), keyboard_(data.keys()), flow_(*this) {}

void Game::newGame(const NewGameSettings& settings) {
    session_.emplace(data_.rules(), settings.players, settings.difficulty, settings.firstLevel,
                     world::RandomNumbers(settings.randomSeed));
    level_ = world::Level();
    level_.rain().setFloorRow(settings.rainFloorRow);
    keyboard_ = input::PcKeyboard(data_.keys());
    flow_.restart();
}

void Game::scancode(uint8_t code) { keyboard_.deliver(code); }

void Game::startGame() { session_->startGame(); }

void Game::startAttempt() {}

void Game::beforePlay() {}

void Game::playFrame() {}

bool Game::attemptOver() const { return false; }

GameResult Game::endAttempt() { return GameResult::Continue; }

}  // namespace ugh::game
