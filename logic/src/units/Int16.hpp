// A 16-bit integer that wraps like a register of the original.
#pragma once

#include <compare>
#include <cstdint>

namespace ugh::units {

/**
 * A signed 16-bit integer that wraps on overflow: 32767 + 1 is -32768. The game computes in 16 bits; this type cuts
 * every result the same way without a cast on every line.
 *
 * - An int converts to an Int16 implicitly and is cut to 16 bits.
 * - `<` and `>` compare signed; unsignedLess() compares the same bits unsigned.
 * - `>>` shifts arithmetically (keeps the sign), `<<` shifts left and wraps.
 */
class Int16 {
public:
    constexpr Int16() = default;
    constexpr Int16(int value) : value_(static_cast<int16_t>(value)) {}

    /** The signed value. */
    constexpr int value() const { return value_; }
    /** The same 16 bits, unsigned. */
    constexpr int bits() const { return static_cast<uint16_t>(value_); }

    constexpr Int16 operator-() const { return Int16(-value_); }
    constexpr Int16 operator>>(int shift) const { return Int16(value_ >> shift); }
    constexpr Int16 operator<<(int shift) const { return Int16(value_ * (1 << shift)); }

    constexpr Int16& operator+=(Int16 other) { return *this = *this + other; }
    constexpr Int16& operator-=(Int16 other) { return *this = *this - other; }

    friend constexpr Int16 operator+(Int16 a, Int16 b) { return Int16(a.value_ + b.value_); }
    friend constexpr Int16 operator-(Int16 a, Int16 b) { return Int16(a.value_ - b.value_); }
    friend constexpr bool operator==(Int16 a, Int16 b) { return a.value_ == b.value_; }
    friend constexpr std::strong_ordering operator<=>(Int16 a, Int16 b) { return a.value_ <=> b.value_; }

    /** a < b, both taken as unsigned 16-bit numbers. */
    static constexpr bool unsignedLess(Int16 a, Int16 b) { return a.bits() < b.bits(); }

private:
    int16_t value_ = 0;
};

}  // namespace ugh::units
