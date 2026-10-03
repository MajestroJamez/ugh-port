#include "game/Attempts.hpp"

#include "game/PlayFrame.hpp"

namespace ugh::game {

void Attempts::startGame() { state_.session->startGame(); }

/** The level exists: the game starts at one (newGame checks it) and ends after the last (end). */
void Attempts::start() {
    world::session::Session& session = *state_.session;
    const data::levels::LevelDefinition& definition = *data_.level(session.players(), session.levelNumber());
    state_.level.startAttempt(definition, data_.sprites(), session.random(), diagnostics_);
    state_.passengers.load(definition, state_.level);
    state_.enemies.load(definition, data_, state_.level);
}

/**
 * The play starts: the enemies, then the passengers get their first update before anything is shown; then nothing is
 * shown.
 */
void Attempts::beforePlay() {
    world::PlayContext c = context();
    state_.enemies.update({c, state_.passengers, state_.bonuses});
    state_.passengers.update({c, state_.bonuses});
    state_.enemies.hideAll();
    state_.passengers.hideAll();
    state_.bonuses.clear();
}

void Attempts::playFrame() {
    world::PlayContext c = context();
    PlayFrame(c, state_.menu, state_.passengers, state_.enemies, state_.bonuses).run();
}

bool Attempts::over() const { return state_.level.fade().over(); }

GameResult Attempts::end() {
    world::session::Session& session = *state_.session;
    if (state_.level.delivery().done())
        return session.nextLevel(data_.levelCount(session.players())) ? GameResult::Continue
                                                                      : GameResult::AllLevelsDone;
    return session.loseLife() ? GameResult::Continue : GameResult::GameOver;
}

world::PlayContext Attempts::context() { return {state_.level, *state_.session, data_, events_, diagnostics_}; }

}  // namespace ugh::game
