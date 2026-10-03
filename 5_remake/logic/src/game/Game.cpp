#include "game/Game.hpp"


namespace ugh::game {

Game::Game(const data::GameData& data) : data_(data), flow_(*this) {}

bool Game::newGame(const NewGameSettings& settings) {
    if (settings.players < 1 || settings.players > 2 || settings.firstLevel < 0 ||
        settings.firstLevel >= data_.levelCount(settings.players) || settings.rainFloorRow < 0 || settings.rainFloorRow > 255)
        return false;
    session_.emplace(data_.rules(), settings.players, settings.difficulty, settings.firstLevel,
                     world::RandomNumbers(settings.randomSeed));
    level_ = world::Level(settings.players);
    passengers_ = passengers::Passengers();
    enemies_ = enemies::Enemies();
    bonuses_.clear();
    level_.rain().setFloorRow(settings.rainFloorRow);
    menu_ = input::MenuInput();
    diagnostics_.take();
    flow_.restart();
    return true;
}

bool Game::levelLoaded() const {
    GamePhase p = phase();
    return p == GamePhase::Caption || p == GamePhase::Setup || p == GamePhase::Play;
}

}  // namespace ugh::game
