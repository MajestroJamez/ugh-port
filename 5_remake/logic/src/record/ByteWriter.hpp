// Writes the bytes of a replay.
#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace ugh::record {

/** Writes bytes: single ones, little-endian words, variable-length numbers (LEB128: 7 bits a byte), short texts. */
class ByteWriter {
public:
    void byte(uint8_t value) { bytes_.push_back(value); }
    void word16(uint16_t value) {
        byte(static_cast<uint8_t>(value));
        byte(static_cast<uint8_t>(value >> 8));
    }
    void word32(uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) byte(static_cast<uint8_t>(value >> shift));
    }
    void number(uint64_t value) {
        while (value >= 0x80) {
            byte(static_cast<uint8_t>(value | 0x80));
            value >>= 7;
        }
        byte(static_cast<uint8_t>(value));
    }
    /** A signed number as an unsigned one, small near 0 (zigzag: 0, -1, 1, -2 ... -> 0, 1, 2, 3 ...). */
    void signedNumber(int64_t value) {
        number(value < 0 ? (static_cast<uint64_t>(-(value + 1)) << 1) | 1 : static_cast<uint64_t>(value) << 1);
    }
    /** Its length, then its bytes. */
    void text(std::string_view value) {
        number(value.size());
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    const std::vector<uint8_t>& bytes() const { return bytes_; }

private:
    std::vector<uint8_t> bytes_;
};

}  // namespace ugh::record
