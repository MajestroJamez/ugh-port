#include "TestData.hpp"
#include "TestFramework.hpp"
#include "game/Game.hpp"

using namespace ugh;

TEST(a_new_game_starts_after_eight_black_frames) {
    game::Game g(test::gameData());
    g.newGame({});
    CHECK(g.phase() == game::GamePhase::Start);
    CHECK_EQUAL(0, g.session().lives());
    g.step();
    CHECK(g.phase() == game::GamePhase::BetweenLevels);
    CHECK_EQUAL(3, g.session().lives());
    CHECK_EQUAL(1, g.session().multiplier());
    for (int frame = 2; frame <= 8; frame++) g.step();
    CHECK(g.phase() == game::GamePhase::BetweenLevels);
    g.step();
    CHECK(g.phase() == game::GamePhase::Caption);
}
