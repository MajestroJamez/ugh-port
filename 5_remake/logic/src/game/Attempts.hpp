// What the phases of the game flow do to the game.
#pragma once

#include "events/Event.hpp"
#include "game/GameResult.hpp"
#include "input/MenuInput.hpp"
#include "world/PlayContext.hpp"

namespace ugh::game {

class Game;

/**
 * What the phases of the game flow do to the game: start the game, start an attempt at a level, play its frames and
 * end it. Only GameFlow has it, so that the facade Game keeps only what a frontend and the replays use.
 */
class Attempts {
public:
    explicit Attempts(Game& game) : game_(game) {}

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

    input::MenuInput& menu();
    void report(const events::Event& event);

private:
    Game& game_;

    world::PlayContext context();
};

}  // namespace ugh::game
