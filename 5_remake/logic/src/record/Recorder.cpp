#include "record/Recorder.hpp"

namespace ugh::record {

void Recorder::onEvent(const events::Event& event) {
    if (event.kind == events::EventKind::LevelCaption) attemptStarted_ = true;
}

void Recorder::reset() {
    current_.reset();
    finished_.reset();
    attemptStarted_ = false;
}

void Recorder::key(int player, input::PlayerKey key, bool pressed) {
    if (current_) add(Input::pilot(current_->steps - 1, player, key, pressed));
}

void Recorder::menuKey(input::MenuKey key) {
    if (current_) add(Input::menu(current_->steps - 1, key));
}

void Recorder::add(const Input& input) { current_->inputs.push_back(input); }

/**
 * The step counts for the level being played; the step an attempt starts in is another attempt at it, or the first of
 * the next level (this step is the last of the level before and the first of the next).
 */
void Recorder::stepped(const game::Game& game, game::GameResult result) {
    const bool started = attemptStarted_;
    attemptStarted_ = false;
    if (current_) current_->steps++;
    if (started && game.attemptStart()) {
        const game::AttemptStart& start = *game.attemptStart();
        if (current_ && current_->start.level == start.level) {
            current_->attempts++;
        } else {
            if (current_) {   // the game went on to the next level: this one was done, with what it took over
                current_->points = start.points - current_->start.points;
                close(true);
            }
            begin(start);
        }
    }
    if (!current_) return;
    if (game.phase() == game::GamePhase::Play) current_->playSteps++;
    current_->points = game.session().score().points() - current_->start.points;
    if (result != game::GameResult::Continue) close(result == game::GameResult::AllLevelsDone);
}

void Recorder::begin(const game::AttemptStart& start) {
    current_.emplace();
    current_->dataHash = dataHash_;
    current_->start = start;
    current_->steps = 1;
    current_->attempts = 1;
}

void Recorder::close(bool done) {
    current_->done = done;
    finished_ = std::move(current_);
    current_.reset();
}

}  // namespace ugh::record
