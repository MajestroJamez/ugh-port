// The game (Facade): the one entry for the C API.
#pragma once

#include <coroutine>
#include <cstdint>

#include "core/Diagnostics.hpp"
#include "core/EventBroadcast.hpp"
#include "core/EventListener.hpp"
#include "data/GameData.hpp"
#include "game/Flow.hpp"
#include "input/Keyboard.hpp"
#include "model/GameSession.hpp"
#include "model/Level.hpp"
#include "physics/CopterPhysics.hpp"

namespace ugh::game {

/**
 * The game: the session and the running level, the keyboard, the physics, and the flow of the original from a new
 * game to its end (113b:0c61 .. 0fe7, GameFlow.kt / Level.kt / Host.kt), without the drawing, the menus and the
 * pause. Every routine names the routine of the original (113b:xxxx) and its Kotlin port
 * (core/src/main/kotlin/ugh/core/game).
 */
class Game {
public:
    explicit Game(const data::GameData& data);
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /** Back to the program start: one player, medium difficulty, level 1, nothing else known. */
    void reset();

    /** Events go to `listener` from now on. */
    void addListener(core::EventListener& listener) { events_.add(listener); }

    /** A scancode from the keyboard interrupt, between two frames. */
    void key(uint8_t scancode) { keyboard_.deliver(scancode, level_); }

    /**
     * The game from the start of 113b:0c61 (GameFlow.kt playGame) to the next place where it waits for the vertical
     * retrace: one frame of the original. UGH_SIM_CONTINUE while waiting there, else how the game ended.
     */
    int step();

    // single transitions from a state set from outside (the replay player checks them one by one)

    /** 113b:3961 - GameFlow.kt newGame. */
    void newGame() { session_.newGame(); }
    /** 113b:0fa7 (GameFlow.kt playGame after playLevel): the next level, or one life less. */
    int levelEnd();
    /** The level setup up to the caption's first retrace wait. */
    void levelStart();
    /** One frame of the level play after the retrace wait that starts it. */
    void playFrame();

    model::Level& level() { return level_; }
    const model::Level& level() const { return level_; }
    core::Diagnostics& diagnostics() { return diagnostics_; }

private:
    const data::GameData& data_;
    core::EventBroadcast events_;
    core::Diagnostics diagnostics_;
    model::GameSession session_;
    model::Level level_;
    input::Keyboard keyboard_;
    physics::CopterPhysics physics_;

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

}  // namespace ugh::game
