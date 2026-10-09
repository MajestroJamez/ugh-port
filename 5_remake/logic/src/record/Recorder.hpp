// Records the replays of the levels of a game.
#pragma once

#include <cstdint>
#include <optional>

#include "events/EventListener.hpp"
#include "game/Game.hpp"
#include "game/GameResult.hpp"
#include "record/Recording.hpp"

namespace ugh::record {

/**
 * Records the replays of the levels of a game: the inputs between its steps, from the step a level's first attempt
 * started in (the caption's event, Observer) to the step the level ended in - the next level's attempt started in it,
 * or the game ended (done when it ended with all levels done). Whoever gives the game its inputs and steps it tells the
 * recorder too. Inputs before a level's first attempt are not kept: what of them lasts is in its start
 * (`game::AttemptStart`).
 */
class Recorder : public events::EventListener {
public:
    explicit Recorder(uint32_t dataHash) : dataHash_(dataHash) {}

    void onEvent(const events::Event& event) override;

    /** A new or resumed game: nothing recorded. */
    void reset();
    void key(int player, input::PlayerKey key, bool pressed);
    void menuKey(input::MenuKey key);
    /** After a step of `game` that returned `result`. */
    void stepped(const game::Game& game, game::GameResult result);

    /** The level being played so far (its points so far, not done yet). */
    const std::optional<Recording>& current() const { return current_; }
    /** The last level that ended. */
    const std::optional<Recording>& finished() const { return finished_; }

private:
    uint32_t dataHash_;
    std::optional<Recording> current_;
    std::optional<Recording> finished_;
    bool attemptStarted_ = false;   // a caption came in this step

    void begin(const game::AttemptStart& start);
    void close(bool done);
    void add(const Input& input);
};

}  // namespace ugh::record
