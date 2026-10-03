// The random numbers of the original (113b:4f09, Draw.kt random).
#pragma once

#include <array>
#include <cstdint>

namespace ugh::core {

/** Four words chained by additions with carry (CS:4ef7 .. 4efd); the game draws from them for the raindrops. */
class Random {
public:
    using Snapshot = std::array<uint16_t, 4>;

    /** 113b:4f09 - a number 0 .. range - 1. */
    uint16_t next(uint16_t range);

    /** The four words, the first one first (the memento for the replays). */
    const Snapshot& snapshot() const { return words_; }
    void restore(const Snapshot& words) { words_ = words; }

private:
    Snapshot words_{};
};

}  // namespace ugh::core
