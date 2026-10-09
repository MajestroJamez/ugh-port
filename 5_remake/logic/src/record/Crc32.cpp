#include "record/Crc32.hpp"

#include <array>
#include <fstream>
#include <iterator>
#include <vector>

namespace ugh::record {

namespace {

constexpr std::array<uint32_t, 256> table() {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int bit = 0; bit < 8; bit++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        t[i] = c;
    }
    return t;
}

constexpr std::array<uint32_t, 256> TABLE = table();

}  // namespace

uint32_t Crc32::of(std::span<const uint8_t> bytes) {
    uint32_t c = 0xFFFFFFFFu;
    for (uint8_t b : bytes) c = TABLE[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

uint32_t Crc32::ofFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return of(bytes);
}

}  // namespace ugh::record
