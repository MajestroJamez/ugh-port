#include "TestFramework.hpp"
#include "core/Fixed.hpp"
#include "core/Speed.hpp"

using ugh::core::Fixed;
using ugh::core::Speed;

TEST(speedPerFrameRoundsDown) {
    CHECK(Speed(-0x41).perFrame() == Fixed(-2));         // SAR 6 rounds down
    CHECK(Speed(0x40).perFrame() == Fixed(1));
    CHECK((Speed(-0x3f) >> 1) == Speed(-0x20));
}

TEST(speedIsClampedBothWays) {
    CHECK(Speed(0x2000).clamped(Speed(0x1800)) == Speed(0x1800));
    CHECK(Speed(-0x2000).clamped(Speed(0x1800)) == Speed(-0x1800));
}
