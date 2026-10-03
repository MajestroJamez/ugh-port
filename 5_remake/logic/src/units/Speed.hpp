// A speed in 1/64 of a Fixed per frame.
#pragma once

#include <compare>

#include "units/Fixed.hpp"
#include "units/Int16.hpp"

namespace ugh::units {

/**
 * The speed of a copter or of a swimmer: in 1/64 of a Fixed per frame, so that a copter can speed up by less than
 * 1/32 px per frame. perFrame() is how far it gets in one frame.
 */
class Speed {
public:
    constexpr Speed() = default;

    /** From the raw value in 1/64 Fixed per frame. */
    static constexpr Speed fromRaw(int raw) { return Speed(Int16(raw)); }

    constexpr int raw() const { return raw_.value(); }

    /** The distance of one frame (the raw value shifted right by 6). */
    constexpr Fixed perFrame() const { return Fixed::fromRaw((raw_ >> 6).value()); }

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
    constexpr explicit Speed(Int16 raw) : raw_(raw) {}

    Int16 raw_;
};

}  // namespace ugh::units
