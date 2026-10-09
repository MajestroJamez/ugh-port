#include "game/Game.hpp"

namespace ugh::game {

Game::Game(const data::GameData& data) : data_(data), flow_(Attempts(data, state_, events_, diagnostics_)) {}

bool Game::newGame(const NewGameSettings& settings) {
    if (!accepts(settings)) return false;
    state_.reset(data_.rules(), settings);
    diagnostics_.take();
    flow_.restart();
    return true;
}

bool Game::resume(const AttemptStart& start) {
    NewGameSettings settings;
    settings.players = start.players;
    settings.difficulty = start.difficulty;
    settings.firstLevel = start.level;
    settings.randomSeed = start.random;
    settings.rainFloorRow = start.rainFloorRow;
    if (!accepts(settings) || start.lives < 0 || start.lives > world::session::Lives::MAX || start.multiplier < 1 ||
        start.multiplier > data_.rules().multiplierLimit(start.difficulty))
        return false;
    if (start.lastMenuKey != input::MenuKey::Escape && start.lastMenuKey != input::MenuKey::Pause &&
        start.lastMenuKey != input::MenuKey::Other)
        return false;
    state_.reset(data_.rules(), settings);
    state_.session->resumeGame(start.lives, start.points, start.multiplier);
    for (world::copter::Copter& copter : state_.level.copters().all())
        copter.rotor().resumeEffort(start.effort[copter.player()]);
    state_.menu = input::MenuInput(start.lastMenuKey);
    diagnostics_.take();
    flow_.resume();
    return true;
}

bool Game::accepts(const NewGameSettings& settings) const {
    if (settings.players < 1 || settings.players > 2 || settings.firstLevel < 0 ||
        settings.firstLevel >= data_.levelCount(settings.players) || settings.rainFloorRow < 0 || settings.rainFloorRow > 255)
        return false;
    return settings.difficulty >= data::Difficulty::Easy && settings.difficulty <= data::Difficulty::Hard;
}

bool Game::levelLoaded() const {
    GamePhase p = phase();
    return p == GamePhase::Caption || p == GamePhase::Setup || p == GamePhase::Play;
}

}  // namespace ugh::game
