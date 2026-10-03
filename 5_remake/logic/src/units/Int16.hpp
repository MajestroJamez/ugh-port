// A 16-bit integer that wraps like a register of the original.
#pragma once

#include <compare>
#include <cstdint>

namespace ugh::units {

/**
 * A signed 16-bit integer that wraps on overflow: 32767 + 1 is -32768. `Fixed` and `Speed` compute with it: positions
 * and speeds keep the 16 bits of the original; so does the named quirk of `world::Energy`. Everything else of the game
 * is a plain int (the replays never overflow 16 bits there, 2_reverse_engineering/notes/rewrite-audit.md).
 *
 * - An int converts to an Int16 implicitly and is cut to 16 bits.
 * - `>>` shifts arithmetically (keeps the sign), `<<` shifts left and wraps.
 */
class Int16 {
public:
    constexpr Int16() = default;
    constexpr Int16(int value) : value_(static_cast<int16_t>(value)) {}   // cut to 16 bits, kept as an int

    /** The signed value. */
    constexpr int value() const { return value_; }

    constexpr Int16 operator-() const { return Int16(-value_); }
    constexpr Int16 operator>>(int shift) const { return Int16(value_ >> shift); }
    constexpr Int16 operator<<(int shift) const { return Int16(value_ * (1 << shift)); }
    constexpr Int16& operator+=(Int16 other) { return *this = *this + other; }
    constexpr Int16& operator-=(Int16 other) { return *this = *this - other; }

    friend constexpr Int16 operator+(Int16 a, Int16 b) { return Int16(a.value_ + b.value_); }
    friend constexpr Int16 operator-(Int16 a, Int16 b) { return Int16(a.value_ - b.value_); }
    friend constexpr bool operator==(Int16 a, Int16 b) { return a.value_ == b.value_; }
    friend constexpr std::strong_ordering operator<=>(Int16 a, Int16 b) { return a.value_ <=> b.value_; }

private:
    // an int, not an int16_t: MSVC of the Build Tools 2026 miscompiles <=> of an int16_t after a negation and a
    // shift (CopterPhysics::bounceVertically took every bounce off a floor for one off a ceiling)
    int value_ = 0;
};

}  // namespace ugh::units
