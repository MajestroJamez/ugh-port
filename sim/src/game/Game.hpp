// The game (Facade): the one entry for the C API.
#pragma once

#include <cstdint>

#include "core/Diagnostics.hpp"
#include "core/EventBroadcast.hpp"
#include "core/EventListener.hpp"
#include "core/Word.hpp"
#include "data/GameData.hpp"
#include "game/GameFlow.hpp"
#include "game/PlayFrame.hpp"
#include "input/Keyboard.hpp"
#include "model/GameSession.hpp"
#include "model/Level.hpp"

namespace ugh::game {

/**
 * The game: the session and the running level, the keyboard, one frame of the play, and the flow of the original
 * from a new game to its end (113b:0c61 .. 0fe7, GameFlow.kt / Level.kt / Host.kt), without the drawing, the menus
 * and the pause. Every routine names the routine of the original (113b:xxxx) and its Kotlin port
 * (core/src/main/kotlin/ugh/core/game).
 */
class Game {
public:
    static constexpr core::Word CAPTION_WATER_ROW = 0xaf;   // the water row while the caption screen is shown

    explicit Game(const data::GameData& data);
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /** Back to the program start: one player, medium difficulty, level 1, nothing else known. */
    void reset();

    /** Events go to `listener` from now on. */
    void addListener(core::EventListener& listener) { events_.add(listener); }

    /** A scancode from the keyboard interrupt, between two frames. */
    void key(uint8_t scancode) { keyboard_.deliver(scancode, level_); }

    /** One frame of the original (GameFlow): UGH_SIM_CONTINUE while the game goes on, else how it ended. */
    int step() { return flow_.step(); }

    // single transitions, also from a state set from outside (the replay player checks them one by one)

    /** 113b:3961 - GameFlow.kt newGame. */
    void newGame() { session_.newGame(); }
    /** 113b:0fa7 (GameFlow.kt playGame after playLevel): the next level, or one life less. */
    int levelEnd();
    /** The level setup up to the caption's first retrace wait (without the caption's event). */
    void levelStart();
    /** 113b:0c7d .. 0fa4 - one frame of the level play after the retrace wait that starts it. */
    void playFrame();

    model::Level& level() { return level_; }
    const model::Level& level() const { return level_; }
    input::Keyboard& keyboard() { return keyboard_; }
    core::Diagnostics& diagnostics() { return diagnostics_; }

private:
    core::EventBroadcast events_;
    core::Diagnostics diagnostics_;
    model::GameSession session_;
    model::Level level_;
    input::Keyboard keyboard_;
    PlayFrame frame_;
    GameFlow flow_;
};

}  // namespace ugh::game
