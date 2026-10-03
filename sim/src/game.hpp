// The game: the state (World), the data, the services, and the flow of the original from a new game to its end.
//
// The rules live in their own files and work on a Game: copter.cpp (physics), water.cpp (water and rain),
// level.cpp (level setup), passengers.cpp, enemies.cpp, bonuses.cpp (the state machines). Every routine names the
// routine of the original (113b:xxxx) and its Kotlin port (core/src/main/kotlin/ugh/core/game).
#pragma once

#include <coroutine>
#include <string>
#include <vector>

#include "data.hpp"
#include "events.hpp"
#include "flow.hpp"
#include "keyboard.hpp"
#include "world.hpp"

namespace ugh {

class Game {
public:
    explicit Game(const GameData& data);
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    const GameData& data;
    World world;
    Keyboard keyboard;
    Events events;
    /** Situations the core does not support (pause, all bonus slots in use); the replay player reports them. */
    std::vector<std::string> problems;

    /** Back to the program start (the state is up to the caller). */
    void reset();

    /** A scancode from the keyboard interrupt, between two frames. */
    void key(uint8_t scancode) { keyboard.deliver(scancode, world.copters); }

    /**
     * The game from the start of 113b:0c61 (GameFlow.kt playGame) to the next place where it waits for the vertical
     * retrace: one frame of the original. UGH_SIM_CONTINUE while waiting there, else how the game ended.
     */
    int step();

    // single transitions from a state set from outside (the replay player checks them one by one)
    void newGame();
    int levelEnd();
    void levelStart();
    void playFrame();

    /** The level being played; World::level follows from the level number and the players. */
    const LevelDefinition& level() const { return *world.level; }
    void addScore(uint32_t points) { world.score += points; }
    void report(Event e) { events.push_back(e); }

private:
    Task game_;
    std::coroutine_handle<> waiting_;
    int result_ = 0;

    Retrace vsync() { return Retrace{&waiting_}; }
    Task playGame();
    Task levelSetup();
    Task levelCaption();
    Task playLevel();
    Task blackPalette();
    Task fadeIn();
    Task fadeOut();
    Task waitKey();
    void frame();
};

/** The first copter touching the box of a sprite at x, y (113b:2276, 22f1, 2207); -1 = none. */
int touchingCopter(const World& world, const Box& box, Fixed x, Fixed y);

// copter.cpp
void flyCopter(Game& game, int player);
void spinRotor(Game& game, int player);

// water.cpp
void moveWater(World& world);
void animateWaterSurface(World& world);
void startRain(Game& game);
void moveRain(Game& game);

// level.cpp
void startLevelAttempt(Game& game);

// passengers.cpp, enemies.cpp, bonuses.cpp
void updatePassengers(Game& game);
void updatePassengerPixels(World& world);
void updateEnemies(Game& game);
void updateBonuses(Game& game);

}  // namespace ugh
