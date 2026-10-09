// Reads the bytes of a replay.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace ugh::record {

/**
 * Reads what ByteWriter wrote. Reading past the end, a number longer than 64 bits or a value outside what the caller
 * allows fails the reader (`ok()` false from then on; the values read then are 0).
 */
class ByteReader {
public:
    explicit ByteReader(std::span<const uint8_t> bytes) : bytes_(bytes) {}

    uint8_t byte() {
        if (!ok_ || at_ >= bytes_.size()) return static_cast<uint8_t>(fail());
        return bytes_[at_++];
    }
    uint16_t word16() {
        uint16_t low = byte();
        return static_cast<uint16_t>(low | byte() << 8);
    }
    uint32_t word32() {
        uint32_t value = 0;
        for (int shift = 0; shift < 32; shift += 8) value |= static_cast<uint32_t>(byte()) << shift;
        return value;
    }
    uint64_t number() {
        uint64_t value = 0;
        for (int shift = 0; shift < 64; shift += 7) {
            uint8_t b = byte();
            value |= static_cast<uint64_t>(b & 0x7F) << shift;
            if (!(b & 0x80)) return ok_ ? value : 0;
        }
        return fail();
    }
    /** A number from `low` to `high`. */
    int64_t number(int64_t low, int64_t high) {
        uint64_t value = number();
        if (value > static_cast<uint64_t>(high) || static_cast<int64_t>(value) < low) return static_cast<int64_t>(fail());
        return static_cast<int64_t>(value);
    }
    int64_t signedNumber(int64_t low, int64_t high) {
        uint64_t raw = number();
        int64_t value = raw & 1 ? -static_cast<int64_t>(raw >> 1) - 1 : static_cast<int64_t>(raw >> 1);
        if (value < low || value > high) return static_cast<int64_t>(fail());
        return value;
    }
    /** A text of at most `maxBytes`, none below a space (a line break, a control). */
    std::string text(size_t maxBytes) {
        size_t size = static_cast<size_t>(number(0, static_cast<int64_t>(maxBytes)));
        std::string value;
        for (size_t i = 0; i < size && ok_; i++) {
            uint8_t b = byte();
            if (b < 0x20 || b == 0x7F) {
                fail();
                return {};
            }
            value.push_back(static_cast<char>(b));
        }
        return ok_ ? value : std::string();
    }

    bool ok() const { return ok_; }
    /** Every byte was read. */
    bool atEnd() const { return at_ == bytes_.size(); }

private:
    std::span<const uint8_t> bytes_;
    size_t at_ = 0;
    bool ok_ = true;

    uint64_t fail() {
        ok_ = false;
        return 0;
    }
};

}  // namespace ugh::record
