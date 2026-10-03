#include "TestFramework.hpp"
#include "core/Countdown.hpp"

TEST(countdownEndsAtZeroAndRunsOnFromIt) {
    ugh::core::Countdown c;
    c.start(2);
    CHECK(!c.tick());
    CHECK(c.tick());                                     // done on the second tick
    CHECK(c.remaining() == 0);
    CHECK(!c.tick() && c.remaining() == -1);             // from zero it runs through the whole word
}
