#include "TestFramework.hpp"
#include "units/Countdown.hpp"
#include "units/Fixed.hpp"
#include "units/Int16.hpp"
#include "units/Speed.hpp"

using namespace ugh::units;

TEST(int16_wraps_like_a_register) {
    CHECK_EQUAL(-32768, (Int16(32767) + 1).value());
    CHECK_EQUAL(32767, (Int16(-32768) - 1).value());
    CHECK_EQUAL(-1, Int16(0xffff).value());
    CHECK_EQUAL(0xffff, Int16(-1).bits());
}

TEST(int16_compares_signed_and_unsigned) {
    CHECK(Int16(-1) < Int16(1));
    CHECK(Int16::unsignedLess(Int16(1), Int16(-1)));
    CHECK(!Int16::unsignedLess(Int16(-1), Int16(1)));
}

TEST(int16_shifts_keep_the_sign) {
    CHECK_EQUAL(-2, (Int16(-3) >> 1).value());
    CHECK_EQUAL(-32768, (Int16(0x4000) << 1).value());
}

TEST(fixed_is_a_thirty_second_of_a_pixel) {
    CHECK_EQUAL(320, Fixed::fromPixels(10).raw().value());
    CHECK_EQUAL(10, Fixed::fromRaw(351).pixels().value());
    CHECK_EQUAL(-1, Fixed::fromRaw(-1).pixels().value());
    CHECK_EQUAL(320, Fixed::fromRaw(351).wholePixel().raw().value());
}

TEST(speed_moves_a_sixty_fourth_per_frame) {
    CHECK_EQUAL(2, Speed::fromRaw(130).perFrame().raw().value());
    CHECK_EQUAL(-3, Speed::fromRaw(-130).perFrame().raw().value());
    CHECK_EQUAL(100, Speed::fromRaw(300).clamped(Speed::fromRaw(100)).raw().value());
    CHECK_EQUAL(-100, Speed::fromRaw(-300).clamped(Speed::fromRaw(100)).raw().value());
}

TEST(countdown_acts_on_the_nth_tick) {
    Countdown c;
    c.start(3);
    CHECK(!c.tick());
    CHECK(!c.tick());
    CHECK(c.tick());
    c.start(0);
    CHECK(!c.tick());
    CHECK_EQUAL(-1, c.remaining().value());
}
