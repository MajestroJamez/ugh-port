// The random numbers of the game.
#pragma once

#include <array>
#include <cstdint>

namespace ugh::world {

/**
 * The random number generator of the original: four 16-bit words, each added to the next with carry, the last fed
 * back into the first. The state at the start of a game is an input of the game (NewGameSettings).
 */
class RandomNumbers {
public:
    using Words = std::array<uint16_t, 4>;

    RandomNumbers() = default;
    explicit RandomNumbers(Words words) : words_(words) {}

    /** A number 0 .. range - 1. */
    uint16_t next(uint16_t range);

    const Words& words() const { return words_; }

private:
    Words words_{};
};

}  // namespace ugh::world
