#include "LevelSetup.hpp"
#include "TestFramework.hpp"

TEST(keysOfBothPlayers) {
    ugh::test::LevelSetup s;
    const auto& p0 = s.level.copter(0).controls();
    const auto& p1 = s.level.copter(1).controls();
    s.game.key(0xe0);
    s.game.key(0x48);   // cursor up: player 0
    CHECK(p0.up && !p1.up);
    s.game.key(0x11);   // W: player 1
    CHECK(p1.up);
    s.game.key(0xe0);
    s.game.key(0xc8);   // cursor up released
    CHECK(!p0.up && p1.up);
}

TEST(extendedKeySequences) {
    ugh::test::LevelSetup s;
    const auto& p0 = s.level.copter(0).controls();
    const auto& p1 = s.level.copter(1).controls();
    s.game.key(0xe0);
    s.game.key(0x2a);   // the fake shift of an extended key: nothing
    s.game.key(0x48);   // keypad 8 without the prefix: player 1 up
    CHECK(!p0.up && p1.up);
    s.game.key(0xe0);
    s.game.key(0x1f);   // E0 1F is no key: the sequence starts again
    s.game.key(0x1f);   // S: player 1 right
    CHECK(p1.right && !p0.right);
}
