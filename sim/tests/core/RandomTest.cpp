#include "TestFramework.hpp"
#include "core/Random.hpp"

using ugh::core::Random;

TEST(randomFollowsTheOriginal) {
    Random r;
    CHECK(r.next(0x140) == 3);   // worked by hand: the words add up 0x140, 0x140, 0x140, then 0x280
    CHECK((r.snapshot() == Random::Snapshot{0x280, 0x140, 0x140, 0x140}));
    CHECK(r.next(0x140) == 14);
    r.restore({0xffff, 0xffff, 0, 0});   // the carries run through the chain
    CHECK(r.next(1) == 0);
    CHECK((r.snapshot() == Random::Snapshot{1, 0, 1, 1}));
}

TEST(randomStaysInRange) {
    Random r;
    for (int i = 0; i < 1000; i++) CHECK(r.next(7) < 7);
}
