#include "game/Game.hpp"

#include <utility>

#include "game/LevelLoader.hpp"
#include "ugh_sim.h"

namespace ugh::game {

namespace {

constexpr uint8_t SCANCODE_ESC = 0x01, SCANCODE_P = 0x19;
constexpr core::Word CAPTION_WATER_ROW = 0xaf;   // the water row while the caption screen is shown
constexpr int BLACK_PALETTE_RETRACES = 8;        // 32 colours per retrace
constexpr int FADE_STEP = 4;                     // per retrace, up to Fade::FULL

}  // namespace

Game::Game(const data::GameData& data)
    : data_(data),
      session_(data),
      level_(data, session_, events_, diagnostics_),
      keyboard_(data.keys()),
      physics_(level_) {}

void Game::reset() {
    session_.restore(model::GameSession::Snapshot{});
    session_.random().restore(core::Random::Snapshot{});
    level_.reset();
    keyboard_.reset();
    diagnostics_.clear();
    game_ = Task();
    waiting_ = nullptr;
    result_ = 0;
}

int Game::levelEnd() {
    if (level_.done()) {
        if (!session_.nextLevel()) return UGH_SIM_ALL_LEVELS_DONE;
    } else {
        if (!session_.loseLife()) return UGH_SIM_GAME_OVER;
    }
    return UGH_SIM_CONTINUE;
}

void Game::levelStart() {
    LevelLoader::startAttempt(level_);
    level_.water().setRow(CAPTION_WATER_ROW);
}

void Game::playFrame() {
    level_.fade().advance();
    frame();
}

/** 113b:0ca5 .. 0fa4 - Frame.kt frameBody / frameAfterKeys without the drawing. */
void Game::frame() {
    level_.water().move(level_.waterSpeed());
    uint8_t key = keyboard_.read().scancode;   // 113b:0fe8 frameKeys (keyboard players only)
    if (key == SCANCODE_P) diagnostics_.report("pause (P) is not supported");
    if (key == SCANCODE_ESC) {   // gives up the game
        session_.giveUp();
        if (!level_.fade().fadingOut()) level_.fade().startFadeOut();
    }
    if (!level_.fade().coptersWaiting())
        for (int player = 0; player < level_.copterCount(); player++) physics_.fly(player);
    level_.updatePassengers();
    level_.updateEnemies();
    level_.bonuses().update(level_);

    level_.updatePassengerPixels();
    for (int player = 0; player < level_.copterCount(); player++)
        level_.copter(player).spinRotor(data_.rotorFirst(player), data_.rotorEnd(player));
    if (level_.windy()) level_.rain().move(level_.water().row(), level_.wind(), session_.random(), diagnostics_);
    level_.water().animateSurface();
    level_.rain().stopAt(level_.water().row());
}

// ---------------------------------------------------------------- the game flow

int Game::step() {
    if (game_.done() && !waiting_) {
        if (result_ != UGH_SIM_CONTINUE) return result_;
        game_ = playGame();
        game_.start();
    } else if (waiting_) {
        std::exchange(waiting_, nullptr).resume();
    }
    return waiting_ ? UGH_SIM_CONTINUE : result_;
}

/** 113b:0c61 .. 0fe7 - GameFlow.kt playGame: a new game, then level attempts until the game is over. */
Task Game::playGame() {
    newGame();
    co_await blackPalette();
    while (true) {
        co_await levelSetup();
        co_await playLevel();   // the music starts before it, the sound stops after it
        int end = levelEnd();
        if (end != UGH_SIM_CONTINUE) {
            result_ = end;
            co_return;
        }
    }
}

/** 113b:3d66 - Level.kt levelSetup: the level, its caption, and the first update of the lists. */
Task Game::levelSetup() {
    LevelLoader::startAttempt(level_);
    co_await levelCaption();
    co_await blackPalette();
    // the game palette, the tiles and the background page are drawn here
    level_.updateEnemies();
    level_.updatePassengers();
    level_.hideSprites();
}

/** 113b:0664 - Level.kt levelCaption: "LEVEL nn", the text and the password, faded in; waits for a key. */
Task Game::levelCaption() {
    core::Word waterRow = level_.water().row();
    level_.water().setRow(CAPTION_WATER_ROW);
    level_.report({core::EventKind::LevelCaption});
    co_await fadeIn();
    co_await waitKey();
    level_.water().setRow(waterRow);
    co_await fadeOut();
}

/** 113b:0c7d .. 0fa4 - GameFlow.kt playLevel: frames until the fade-out at the end of the attempt is done. */
Task Game::playLevel() {
    while (!level_.fade().over()) {
        co_await vsync();   // 113b:4e36: the palette at the fade position
        level_.fade().advance();
        frame();
    }
}

/** 113b:4e9b - Host.kt blackPalette. */
Task Game::blackPalette() {
    for (int i = 0; i < BLACK_PALETTE_RETRACES; i++) co_await vsync();
}

/** 113b:4e19 / 4e28 - Host.kt fadeIn / fadeOut: 65 steps of the palette, one per retrace. */
Task Game::fadeIn() {
    for (core::Word step = 0; step <= model::Fade::FULL; step += FADE_STEP) co_await vsync();
}

Task Game::fadeOut() {
    for (core::Word step = model::Fade::FULL; step >= 0; step -= FADE_STEP) co_await vsync();
}

/** 113b:44db - Host.kt waitKey: until the scancode changes (no joystick). */
Task Game::waitKey() {
    keyboard_.read();
    do co_await vsync();
    while (!keyboard_.read().changed);
}

}  // namespace ugh::game
