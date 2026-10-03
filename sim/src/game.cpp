// The game flow (113b:0c61 .. 0fe7, GameFlow.kt / Level.kt / Host.kt) and the play frame (Frame.kt), without the
// drawing, the menus and the pause.
#include "game.hpp"

#include <utility>

namespace ugh {

namespace {

constexpr uint8_t SCANCODE_ESC = 0x01, SCANCODE_P = 0x19;
constexpr int16_t CAPTION_WATER_ROW = 0xaf;   // the water row while the caption screen is shown
constexpr int16_t FADE_FULL = 0x100;

}  // namespace

Game::Game(const GameData& data) : data(data), keyboard(data.keys()) {}

void Game::reset() {
    world = World{};
    keyboard.reset();
    events.clear();
    problems.clear();
    game_ = Task();
    waiting_ = nullptr;
    result_ = 0;
}

/** 113b:3961 - GameFlow.kt newGame. */
void Game::newGame() {
    world.lives = 3;
    world.multiplier = 1;
    world.score = 0;
}

/** 113b:0fa7 (GameFlow.kt playGame after playLevel): the next level, or one life less and the multiplier back to 1. */
int Game::levelEnd() {
    if (world.levelDone) {
        int next = world.levelNumber + 1;
        world.levelNumber = static_cast<uint16_t>(next);
        if (next >= data.levelCount(world.players)) return UGH_SIM_ALL_LEVELS_DONE;
    } else {
        world.lives--;
        if (world.lives == 0 || (world.lives & 0x80)) return UGH_SIM_GAME_OVER;
        world.multiplier = 1;
    }
    return UGH_SIM_CONTINUE;
}

/** The level setup up to the caption's first retrace wait (for checking single transitions). */
void Game::levelStart() {
    startLevelAttempt(*this);
    world.water.row = CAPTION_WATER_ROW;
}

/** One frame of the level play after the retrace wait that starts it (for checking single transitions). */
void Game::playFrame() {
    if (world.fade.position <= FADE_FULL) world.fade.position = static_cast<int16_t>(world.fade.position + world.fade.step);
    frame();
}

/** 113b:0ca5 .. 0fa4 - Frame.kt frameBody / frameAfterKeys without the drawing. */
void Game::frame() {
    moveWater(world);
    uint8_t key = keyboard.read().scancode;   // 113b:0fe8 frameKeys (keyboard players only)
    if (key == SCANCODE_P) problems.push_back("pause (P) is not supported");
    if (key == SCANCODE_ESC) {   // gives up the game
        world.lives = 0;
        if (!world.fade.fadingOut()) world.fade.startFadeOut();
    }
    // the copters stand still while the level fades in
    if (world.fade.position > 0xc0 || world.fade.step <= 0)
        for (int p = 0; p < world.copterCount(); p++) flyCopter(*this, p);
    updatePassengers(*this);
    updateEnemies(*this);
    updateBonuses(*this);

    updatePassengerPixels(world);
    for (int p = 0; p < world.copterCount(); p++) spinRotor(*this, p);
    if (world.wind != 0) moveRain(*this);
    animateWaterSurface(world);
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
    startLevelAttempt(*this);
    co_await levelCaption();
    co_await blackPalette();
    // the game palette, the tiles and the background page are drawn here
    updateEnemies(*this);
    updatePassengers(*this);
    // 113b:0b4f resetDrawnSprites: nothing is drawn yet
    for (Enemy& e : world.enemies) e.sprite = NO_SPRITE;
    for (Passenger& p : world.passengers) p.sprite = NO_SPRITE;
    for (BonusItem& b : world.bonuses) b.sprite = NO_SPRITE;
}

/** 113b:0664 - Level.kt levelCaption: "LEVEL nn", the text and the password, faded in; waits for a key. */
Task Game::levelCaption() {
    int16_t waterRow = world.water.row;
    world.water.row = CAPTION_WATER_ROW;
    report({EventKind::LevelCaption});
    co_await fadeIn();
    co_await waitKey();
    world.water.row = waterRow;
    co_await fadeOut();
}

/** 113b:0c7d .. 0fa4 - GameFlow.kt playLevel: frames until the fade-out at the end of the attempt is done. */
Task Game::playLevel() {
    while (true) {
        int16_t fade = world.fade.position;
        if (fade < 0) co_return;
        co_await vsync();   // 113b:4e36: the palette at the fade position
        if (fade <= FADE_FULL) world.fade.position = static_cast<int16_t>(fade + world.fade.step);
        frame();
    }
}

/** 113b:4e9b - Host.kt blackPalette: 32 colours per retrace, 8 retraces. */
Task Game::blackPalette() {
    for (int i = 0; i < 8; i++) co_await vsync();
}

/** 113b:4e19 / 4e28 - Host.kt fadeIn / fadeOut: 65 steps of the palette, one per retrace. */
Task Game::fadeIn() {
    for (int step = 0; step <= 0x100; step += 4) co_await vsync();
}

Task Game::fadeOut() {
    for (int step = 0x100; step >= 0; step -= 4) co_await vsync();
}

/** 113b:44db - Host.kt waitKey: until the scancode changes (no joystick). */
Task Game::waitKey() {
    keyboard.read();
    do co_await vsync();
    while (!keyboard.read().changed);
}

}  // namespace ugh
