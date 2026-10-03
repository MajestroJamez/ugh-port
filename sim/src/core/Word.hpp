// A 16-bit word of the original: the arithmetic of the 8086 registers the game computes in.
#pragma once

#include <compare>
#include <cstdint>

namespace ugh::core {

/**
 * A signed 16-bit word that wraps like a register: 0x7fff + 1 is -0x8000. The game logic computes in Words so that
 * every result is cut to 16 bits the way the original's is, without a cast on every line.
 *
 * - An int converts to a Word implicitly and is cut to 16 bits (like MOV into a register).
 * - `<`, `>` compare signed (the original's JL / JG); unsignedLess() compares unsigned (JB / JA).
 * - `>>` shifts arithmetically (SAR), `<<` shifts left and wraps (SHL).
 * - value() is the signed value (an int16_t), bits() the same 16 bits unsigned.
 */
class Word {
public:
    constexpr Word() = default;
    constexpr Word(int value) : value_(static_cast<int16_t>(value)) {}

    constexpr int16_t value() const { return value_; }
    constexpr uint16_t bits() const { return static_cast<uint16_t>(value_); }

    constexpr Word operator-() const { return Word(-value_); }
    constexpr Word operator>>(int shift) const { return Word(value_ >> shift); }
    constexpr Word operator<<(int shift) const { return Word(value_ * (1 << shift)); }
    constexpr Word operator^(Word other) const { return Word(value_ ^ other.value_); }
    constexpr Word operator&(Word other) const { return Word(value_ & other.value_); }

    constexpr Word& operator+=(Word other) { return *this = *this + other; }
    constexpr Word& operator-=(Word other) { return *this = *this - other; }
    constexpr Word& operator++() { return *this += 1; }
    constexpr Word& operator--() { return *this -= 1; }

    friend constexpr Word operator+(Word a, Word b) { return Word(a.value_ + b.value_); }
    friend constexpr Word operator-(Word a, Word b) { return Word(a.value_ - b.value_); }
    friend constexpr bool operator==(Word a, Word b) { return a.value_ == b.value_; }
    friend constexpr std::strong_ordering operator<=>(Word a, Word b) { return a.value_ <=> b.value_; }

    /** a < b as unsigned words (JB). */
    static constexpr bool unsignedLess(Word a, Word b) { return a.bits() < b.bits(); }

private:
    int16_t value_ = 0;
};

}  // namespace ugh::core
