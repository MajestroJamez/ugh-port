#include "core/Random.hpp"

namespace ugh::core {

uint16_t Random::next(uint16_t range) {
    uint32_t sum = static_cast<uint32_t>(range) + words_[0];
    uint32_t ax = sum & 0xffff;
    uint32_t carry = sum >> 16;
    for (int i = 1; i < 4; i++) {
        // each word adds the one before it (and the carry); the last one feeds the first
        uint32_t previous = i == 1 ? ax : words_[i - 1];
        sum = words_[i] + previous + carry;
        carry = sum >> 16;
        words_[i] = static_cast<uint16_t>(sum);
    }
    ax = (ax + words_[3] + carry) & 0xffff;
    words_[0] = static_cast<uint16_t>(ax);
    return static_cast<uint16_t>((ax * range) >> 16);
}

}  // namespace ugh::core
