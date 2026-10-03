#include "game/Attempts.hpp"

#include "game/Game.hpp"
#include "game/PlayFrame.hpp"

namespace ugh::game {

void Attempts::startGame() { game_.session_->startGame(); }

/** The level exists: the game starts at one (newGame checks it) and ends after the last (end). */
void Attempts::start() {
    world::Session& session = *game_.session_;
    const data::LevelDefinition& definition = *game_.data_.level(session.players(), session.levelNumber());
    game_.level_.startAttempt(definition, game_.data_.sprites(), session.random(), game_.diagnostics_);
    game_.passengers_.load(definition);
    game_.enemies_.load(definition, game_.data_);
}

/** The play starts: the enemies, then the passengers get their first update before anything is shown; then nothing is shown. */
void Attempts::beforePlay() {
    world::PlayContext c = context();
    game_.enemies_.update({c, game_.passengers_, game_.bonuses_});
    game_.passengers_.update({c, game_.bonuses_});
    game_.enemies_.hideAll();
    game_.passengers_.hideAll();
    game_.bonuses_.clear();
}

void Attempts::playFrame() {
    game_.level_.fade().advance();
    world::PlayContext c = context();
    PlayFrame(c, game_.menu_, game_.passengers_, game_.enemies_, game_.bonuses_).run();
}

bool Attempts::over() const { return game_.level_.fade().over(); }

GameResult Attempts::end() {
    world::Session& session = *game_.session_;
    if (game_.level_.done())
        return session.nextLevel(game_.data_.levelCount(session.players())) ? GameResult::Continue : GameResult::AllLevelsDone;
    return session.loseLife() ? GameResult::Continue : GameResult::GameOver;
}

input::MenuInput& Attempts::menu() { return game_.menu_; }

void Attempts::report(const events::Event& event) { game_.events_.onEvent(event); }

world::PlayContext Attempts::context() {
    return {game_.level_, *game_.session_, game_.data_, game_.events_, game_.diagnostics_};
}

}  // namespace ugh::game
