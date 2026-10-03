// The initialized data segment of the original (DGROUP) as the extractor exported it.
#pragma once

#include <cstdint>
#include <vector>

namespace ugh::data {

/**
 * The 64 KiB of the original's data segment. Only the loader reads the tables in it (by their offsets); the
 * animations keep reading it while the game runs, because the original reads an animation past its end.
 */
class DataImage {
public:
    /** The marker that ends the word lists of the data (animations, routes, bonus drops). */
    static constexpr uint16_t LIST_END = 0xffff;

    uint16_t word(int offset) const {
        return static_cast<uint16_t>(bytes_[offset & 0xffff] | (bytes_[(offset + 1) & 0xffff] << 8));
    }

    uint8_t byte(int offset) const { return bytes_[offset & 0xffff]; }

    /** Copies a block of the export to its offset; false when it does not fit. */
    bool place(int offset, const uint8_t* block, size_t length) {
        if (offset < 0 || offset + length > bytes_.size()) return false;
        for (size_t i = 0; i < length; i++) bytes_[offset + i] = block[i];
        return true;
    }

private:
    std::vector<uint8_t> bytes_ = std::vector<uint8_t>(0x10000);
};

}  // namespace ugh::data
