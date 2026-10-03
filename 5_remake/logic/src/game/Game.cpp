#include "game/Game.hpp"

#include "game/PlayFrame.hpp"

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

void Game::startGame() { session_->startGame(); }

/** The level exists: the game starts at one (newGame checks it) and ends after the last (endAttempt). */
void Game::startAttempt() {
    const data::LevelDefinition& definition = *data_.level(session_->players(), session_->levelNumber());
    level_.startAttempt(definition, data_.sprites(), session_->random(), diagnostics_);
    passengers_.load(definition);
    enemies_.load(definition, data_);
}

/** The play starts: the enemies, then the passengers get their first update before anything is shown; then nothing is shown. */
void Game::beforePlay() {
    world::PlayContext c = context();
    enemies_.update({c, passengers_, bonuses_});
    passengers_.update({c, bonuses_});
    enemies_.hideAll();
    passengers_.hideAll();
    bonuses_.clear();
}

void Game::playFrame() {
    level_.fade().advance();
    world::PlayContext c = context();
    PlayFrame(c, menu_, passengers_, enemies_, bonuses_).run();
}

GameResult Game::endAttempt() {
    if (level_.done()) return session_->nextLevel(data_.levelCount(session_->players())) ? GameResult::Continue
                                                                                         : GameResult::AllLevelsDone;
    return session_->loseLife() ? GameResult::Continue : GameResult::GameOver;
}

world::PlayContext Game::context() { return {level_, *session_, data_, events_, diagnostics_}; }

}  // namespace ugh::game
