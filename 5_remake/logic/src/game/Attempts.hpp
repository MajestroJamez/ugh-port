// What the phases of the game flow do to the game.
#pragma once

#include "data/GameData.hpp"
#include "events/Diagnostics.hpp"
#include "events/Event.hpp"
#include "events/EventListener.hpp"
#include "game/GameResult.hpp"
#include "game/GameState.hpp"
#include "input/MenuInput.hpp"
#include "world/PlayContext.hpp"

namespace ugh::game {

/**
 * What the phases of the game flow do to the game state: start the game, start an attempt at a level, play its
 * frames and end it. Only GameFlow has it, so that the facade Game keeps only what a frontend and the replays use.
 */
class Attempts {
public:
    Attempts(const data::GameData& data, GameState& state, events::EventListener& events,
             events::Diagnostics& diagnostics)
        : data_(data), state_(state), events_(events), diagnostics_(diagnostics) {}

    /** The game starts: lives, multiplier, score. */
    void startGame();
    /** A new attempt at the current level. */
    void start();
    /** The level lists get their first update, then nothing is shown. */
    void beforePlay();
    /** One frame of the play. */
    void playFrame();
    /** The attempt is over (its fade-out reached black). */
    bool over() const;
    /** The next level when the attempt finished the level, else a life less. */
    GameResult end();

    input::MenuInput& menu() { return state_.menu; }
    void report(const events::Event& event) { events_.onEvent(event); }

private:
    const data::GameData& data_;
    GameState& state_;
    events::EventListener& events_;
    events::Diagnostics& diagnostics_;

    world::PlayContext context();
};

}  // namespace ugh::game
