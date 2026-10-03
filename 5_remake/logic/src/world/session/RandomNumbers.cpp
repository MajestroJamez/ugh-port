#include "world/session/RandomNumbers.hpp"

namespace ugh::world::session {

uint16_t RandomNumbers::next(uint16_t range) {
    uint32_t sum = static_cast<uint32_t>(range) + words_[0];
    uint32_t first = sum & 0xffff;
    uint32_t carry = sum >> 16;
    for (int i = 1; i < 4; i++) {
        // each word adds the one before it and the carry
        uint32_t previous = i == 1 ? first : words_[i - 1];
        sum = words_[i] + previous + carry;
        carry = sum >> 16;
        words_[i] = static_cast<uint16_t>(sum);
    }
    first = (first + words_[3] + carry) & 0xffff;
    words_[0] = static_cast<uint16_t>(first);
    return static_cast<uint16_t>((first * range) >> 16);
}

}  // namespace ugh::world::session
