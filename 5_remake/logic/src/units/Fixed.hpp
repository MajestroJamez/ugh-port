// A position or distance in 1/32 px.
#pragma once

#include <compare>

#include "units/Int16.hpp"

namespace ugh::units {

/**
 * A position, or a distance per frame, in 1/32 px: the fixed point of the game, one 16-bit integer that wraps.
 * Positions are top left corners of sprites, on the screen of play (data::levels::ScreenSize).
 */
class Fixed {
public:
    constexpr Fixed() = default;

    /** From the raw value in 1/32 px. */
    static constexpr Fixed fromRaw(int raw) { return Fixed(Int16(raw)); }
    /** From whole pixels. */
    static constexpr Fixed fromPixels(int pixels) { return Fixed(Int16(pixels) << 5); }

    /** The raw value in 1/32 px. */
    constexpr int raw() const { return raw_.value(); }
    /** Whole pixels, rounded down. */
    constexpr int pixels() const { return (raw_ >> 5).value(); }
    /** Rounded down to a whole pixel. */
    constexpr Fixed wholePixel() const { return Fixed(Int16(raw_.value() & ~31)); }
    /** Half of it, rounded down. */
    constexpr Fixed half() const { return Fixed(raw_ >> 1); }

    constexpr Fixed operator-() const { return Fixed(-raw_); }
    constexpr Fixed& operator+=(Fixed other) { raw_ += other.raw_; return *this; }
    constexpr Fixed& operator-=(Fixed other) { raw_ -= other.raw_; return *this; }
    friend constexpr Fixed operator+(Fixed a, Fixed b) { return Fixed(a.raw_ + b.raw_); }
    friend constexpr Fixed operator-(Fixed a, Fixed b) { return Fixed(a.raw_ - b.raw_); }
    friend constexpr bool operator==(Fixed a, Fixed b) { return a.raw_ == b.raw_; }
    friend constexpr std::strong_ordering operator<=>(Fixed a, Fixed b) { return a.raw_ <=> b.raw_; }

private:
    constexpr explicit Fixed(Int16 raw) : raw_(raw) {}

    Int16 raw_;
};

}  // namespace ugh::units
