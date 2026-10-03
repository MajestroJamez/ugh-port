#include "game/Game.hpp"

#include <string>

#include "game/PlayFrame.hpp"

namespace ugh::game {

Game::Game(const data::GameData& data) : data_(data), keyboard_(data.keys()), flow_(*this) {}

void Game::newGame(const NewGameSettings& settings) {
    session_.emplace(data_.rules(), settings.players, settings.difficulty, settings.firstLevel,
                     world::RandomNumbers(settings.randomSeed));
    level_ = world::Level(settings.players);
    passengers_ = passengers::Passengers();
    bonuses_.clear();
    level_.rain().setFloorRow(settings.rainFloorRow);
    keyboard_ = input::PcKeyboard(data_.keys());
    flow_.restart();
}

void Game::scancode(uint8_t code) {
    const data::KeyBinding::Action* action = keyboard_.deliver(code);
    if (action) level_.copter(action->player).controls().set(action->key, action->press);
}

bool Game::levelLoaded() const {
    GamePhase p = phase();
    return p == GamePhase::Caption || p == GamePhase::Setup || p == GamePhase::Play;
}

void Game::startGame() { session_->startGame(); }

void Game::startAttempt() {
    const data::LevelDefinition* definition = data_.level(session_->players(), session_->levelNumber());
    if (!definition) {
        diagnostics_.report("no level " + std::to_string(session_->levelNumber()));
        return;
    }
    level_.startAttempt(*definition, data_.sprites(), session_->random(), diagnostics_);
    passengers_.load(*definition);
}

/** The play starts: the passengers get their first update before anything is shown; then nothing is shown. */
void Game::beforePlay() {
    world::PlayContext c = context();
    passengers_.update({c, bonuses_});
    passengers_.hideAll();
    bonuses_.clear();
}

void Game::playFrame() {
    level_.fade().advance();
    world::PlayContext c = context();
    PlayFrame(c, keyboard_, passengers_, bonuses_).run();
}

GameResult Game::endAttempt() {
    if (level_.done()) return session_->nextLevel(data_.levelCount(session_->players())) ? GameResult::Continue
                                                                                         : GameResult::AllLevelsDone;
    return session_->loseLife() ? GameResult::Continue : GameResult::GameOver;
}

world::PlayContext Game::context() { return {level_, *session_, data_, events_, diagnostics_}; }

}  // namespace ugh::game
