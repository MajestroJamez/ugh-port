// The arithmetic of the original: 16-bit words that wrap like the 8086 registers.
//
// C++20 defines the conversion of an int to int16_t as wrapping and the right shift of a negative int as
// arithmetic (SAR), so a value kept in an int16_t behaves like a word of the original: compute in int, store,
// done. Fixed adds the unit of the positions. The original compares most words signed (JL / JG); where it
// compares unsigned (JB / JA) the code says so with an explicit cast to uint16_t.
#pragma once

#include <compare>
#include <cstdint>

namespace ugh {

/** A position, or a distance per frame, in 1/32 px: the fixed point of the original, one 16-bit word. */
class Fixed {
public:
    constexpr Fixed() = default;

    /** From the raw word (1/32 px). */
    constexpr explicit Fixed(int raw) : raw_(static_cast<int16_t>(raw)) {}

    /** Whole pixels (SHL 5, wraps). */
    static constexpr Fixed fromPixels(int pixels) { return Fixed(pixels * 32); }

    constexpr int16_t raw() const { return raw_; }

    /** Whole pixels, rounded down (SAR 5). */
    constexpr int16_t pixels() const { return static_cast<int16_t>(raw_ >> 5); }

    /** Rounded down to a whole pixel (AND 0xffe0). */
    constexpr Fixed wholePixel() const { return Fixed(raw_ & ~31); }

    constexpr Fixed operator-() const { return Fixed(-raw_); }
    constexpr Fixed& operator+=(Fixed other) { raw_ = static_cast<int16_t>(raw_ + other.raw_); return *this; }
    constexpr Fixed& operator-=(Fixed other) { raw_ = static_cast<int16_t>(raw_ - other.raw_); return *this; }
    friend constexpr Fixed operator+(Fixed a, Fixed b) { return Fixed(a.raw_ + b.raw_); }
    friend constexpr Fixed operator-(Fixed a, Fixed b) { return Fixed(a.raw_ - b.raw_); }
    friend constexpr auto operator<=>(Fixed, Fixed) = default;

private:
    int16_t raw_ = 0;
};

/** 113b:3d4d - Amiga rows to PC rows (256 to 192): y - (y >> 2). */
constexpr int16_t threeQuarters(int16_t y) { return static_cast<int16_t>(y - (y >> 2)); }

/** 113b:3d5a - x + (x >> 1). */
constexpr int16_t threeHalves(int16_t x) { return static_cast<int16_t>(x + (x >> 1)); }

/** A speed in 1/64 of the fixed point (the copters, a passenger in the water): its movement in one frame (SAR 6). */
constexpr Fixed perFrame(int16_t speed) { return Fixed(speed >> 6); }

}  // namespace ugh
