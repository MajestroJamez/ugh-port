#include "TestData.hpp"
#include "TestFramework.hpp"
#include "game/Game.hpp"

using namespace ugh;

TEST(a_new_game_starts_after_eight_black_frames) {
    game::Game g(test::gameData());
    g.newGame({});
    CHECK(g.phase() == game::GamePhase::Start);
    CHECK_EQUAL(0, g.session().lives().count());
    g.step();
    CHECK(g.phase() == game::GamePhase::BetweenLevels);
    CHECK_EQUAL(world::session::Lives::START, g.session().lives().count());
    CHECK_EQUAL(1, g.session().score().multiplier());
    for (int frame = 2; frame <= 8; frame++) g.step();
    CHECK(g.phase() == game::GamePhase::BetweenLevels);
    g.step();
    CHECK(g.phase() == game::GamePhase::Caption);
}

TEST(the_caption_waits_for_a_key_then_the_play_starts) {
    game::Game g(test::gameData());
    g.newGame({});
    for (int frame = 1; frame <= 9 + 64 + 10; frame++) g.step();
    CHECK(g.phase() == game::GamePhase::Caption);
    CHECK(g.level().definition() != nullptr);
    g.menuKey(input::MenuKey::Other);
    g.step();   // the key: the fade-out starts
    for (int frame = 0; frame < 64; frame++) g.step();
    CHECK(g.phase() == game::GamePhase::Caption);
    g.step();
    CHECK(g.phase() == game::GamePhase::Setup);
    for (int frame = 0; frame < 8; frame++) g.step();
    CHECK(g.phase() == game::GamePhase::Play);
    CHECK_EQUAL(0, g.level().fade().position());
}
