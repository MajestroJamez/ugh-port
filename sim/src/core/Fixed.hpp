// A position of the original: a Word in 1/32 px.
#pragma once

#include <compare>

#include "core/Word.hpp"

namespace ugh::core {

/**
 * A position, or a distance per frame, in 1/32 px: the fixed point of the original, one 16-bit word that wraps.
 * Positions are top left corners; the screen is 320 x 200 px, a little more is kept around it.
 */
class Fixed {
public:
    constexpr Fixed() = default;

    /** From the raw word (1/32 px). */
    constexpr explicit Fixed(Word raw) : raw_(raw) {}

    /** Whole pixels (SHL 5, wraps). */
    static constexpr Fixed fromPixels(Word pixels) { return Fixed(pixels << 5); }

    constexpr Word raw() const { return raw_; }

    /** Whole pixels, rounded down (SAR 5). */
    constexpr Word pixels() const { return raw_ >> 5; }

    /** Rounded down to a whole pixel (AND 0xffe0). */
    constexpr Fixed wholePixel() const { return Fixed(raw_ & ~31); }

    constexpr Fixed operator-() const { return Fixed(-raw_); }
    constexpr Fixed& operator+=(Fixed other) { raw_ += other.raw_; return *this; }
    constexpr Fixed& operator-=(Fixed other) { raw_ -= other.raw_; return *this; }
    friend constexpr Fixed operator+(Fixed a, Fixed b) { return Fixed(a.raw_ + b.raw_); }
    friend constexpr Fixed operator-(Fixed a, Fixed b) { return Fixed(a.raw_ - b.raw_); }
    friend constexpr bool operator==(Fixed a, Fixed b) { return a.raw_ == b.raw_; }
    friend constexpr std::strong_ordering operator<=>(Fixed a, Fixed b) { return a.raw_ <=> b.raw_; }

private:
    Word raw_;
};

}  // namespace ugh::core
