#include "LevelSetup.hpp"
#include "TestFramework.hpp"
#include "ugh_sim.h"

using namespace ugh;

namespace {

constexpr uint8_t SCANCODE_SPACE = 0x39, SCANCODE_ESC = 0x01;

/** Steps the game `frames` times; false when it ended on the way. */
bool run(game::Game& game, int frames) {
    for (int f = 0; f < frames; f++)
        if (game.step() != UGH_SIM_CONTINUE) return false;
    return true;
}

}  // namespace

TEST(phasesTakeTheFramesOfTheOriginal) {
    test::LevelSetup s;
    s.game.reset();
    // a new game, then 8 frames of black before the caption
    CHECK(run(s.game, 8));
    CHECK(!s.reported(core::EventKind::LevelCaption));
    CHECK(run(s.game, 1));
    CHECK(s.reported(core::EventKind::LevelCaption));
    CHECK(s.level.water().row() == game::Game::CAPTION_WATER_ROW);
    // 65 frames of the caption fading in; then it waits for a key
    CHECK(run(s.game, 65 + 100));
    s.game.key(SCANCODE_SPACE);
    // 65 frames of the caption fading out, 8 of black; the first play frame starts the fade
    CHECK(run(s.game, 1 + 65 + 8));
    CHECK(s.level.water().row() == s.level.definition()->water.pixels());   // the level's own row back
    CHECK(s.level.fade().position() == 0);
    CHECK(run(s.game, 1));
    CHECK(s.level.fade().position() == 2);
}

TEST(wholeGameEndsWithEsc) {
    test::LevelSetup s;
    s.game.reset();
    int result = UGH_SIM_CONTINUE;
    for (int frames = 0; frames < 5000 && result == UGH_SIM_CONTINUE; frames++) {
        if (frames == 300) s.game.key(SCANCODE_SPACE);   // past the caption
        if (frames == 500) s.game.key(SCANCODE_ESC);     // gives up
        result = s.game.step();
    }
    CHECK(result == UGH_SIM_GAME_OVER);
    CHECK(s.game.step() == UGH_SIM_GAME_OVER);   // and stays over
}
