// Random numbers of the original (113b:4f09, Draw.kt random): four words chained by additions with carry.
#pragma once

#include <array>
#include <cstdint>

namespace ugh {

class Random {
public:
    /** The state, CS:4ef7 .. 4efd. */
    std::array<uint16_t, 4> state{};

    /** 0 .. range - 1. */
    uint16_t next(uint16_t range) {
        uint32_t sum = static_cast<uint32_t>(range) + state[0];
        uint32_t ax = sum & 0xffff;
        uint32_t carry = sum >> 16;
        for (int i = 1; i < 4; i++) {
            // each word adds the one before it (and the carry); the last one feeds the first
            uint32_t previous = i == 1 ? ax : state[i - 1];
            sum = state[i] + previous + carry;
            carry = sum >> 16;
            state[i] = static_cast<uint16_t>(sum);
        }
        ax = (ax + state[3] + carry) & 0xffff;
        state[0] = static_cast<uint16_t>(ax);
        return static_cast<uint16_t>((ax * range) >> 16);
    }
};

}  // namespace ugh
