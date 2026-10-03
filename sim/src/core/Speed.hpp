// A speed of the original: a Word in 1/64 of the fixed point per frame.
#pragma once

#include <compare>

#include "core/Fixed.hpp"
#include "core/Word.hpp"

namespace ugh::core {

/**
 * The speed of a copter, or of a passenger in the water: in 1/64 of a Fixed per frame, so a copter can speed up
 * by less than 1/32 px per frame. perFrame() is how far it gets in one frame.
 */
class Speed {
public:
    constexpr Speed() = default;

    /** From the raw word (1/64 Fixed per frame). */
    constexpr explicit Speed(Word raw) : raw_(raw) {}

    constexpr Word raw() const { return raw_; }

    /** The distance of one frame (SAR 6). */
    constexpr Fixed perFrame() const { return Fixed(raw_ >> 6); }

    /** Limited to -limit .. limit. */
    constexpr Speed clamped(Speed limit) const { return *this < -limit ? -limit : *this > limit ? limit : *this; }

    constexpr Speed operator-() const { return Speed(-raw_); }
    constexpr Speed operator>>(int shift) const { return Speed(raw_ >> shift); }
    constexpr Speed& operator+=(Speed other) { raw_ += other.raw_; return *this; }
    constexpr Speed& operator-=(Speed other) { raw_ -= other.raw_; return *this; }
    friend constexpr Speed operator+(Speed a, Speed b) { return Speed(a.raw_ + b.raw_); }
    friend constexpr Speed operator-(Speed a, Speed b) { return Speed(a.raw_ - b.raw_); }
    friend constexpr bool operator==(Speed a, Speed b) { return a.raw_ == b.raw_; }
    friend constexpr std::strong_ordering operator<=>(Speed a, Speed b) { return a.raw_ <=> b.raw_; }

private:
    Word raw_;
};

}  // namespace ugh::core
