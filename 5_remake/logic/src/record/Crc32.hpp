// The CRC-32 of bytes.
#pragma once

#include <cstdint>
#include <span>

namespace ugh::record {

/** The CRC-32 of zip and PNG (IEEE 802.3, reflected, polynomial 0xEDB88320): a replay's checksum, the data's hash. */
class Crc32 {
public:
    static uint32_t of(std::span<const uint8_t> bytes);
    /** Of a file's bytes (the game data's hash); of nothing when it cannot be read. */
    static uint32_t ofFile(const char* path);
};

}  // namespace ugh::record
