#include "TestFramework.hpp"
#include "core/AmigaScale.hpp"
#include "core/Fixed.hpp"

using ugh::core::Fixed;

TEST(fixedWrapsAndRoundsDown) {
    CHECK(Fixed(0x7fff) + Fixed(1) == Fixed(-0x8000));   // wraps like a register
    CHECK(Fixed(-1).pixels() == -1);                     // SAR rounds down
    CHECK(Fixed(63).pixels() == 1);
    CHECK(Fixed(-33).wholePixel() == Fixed(-64));
    CHECK(Fixed::fromPixels(0x400) == Fixed(-0x8000));
}

TEST(amigaValuesScaleToThePc) {
    CHECK(ugh::core::amigaRowsToPc(-5) == -3);           // -5 - (-5 >> 2) = -5 + 2
    CHECK(ugh::core::amigaFramesToPc(7) == 10);
}
