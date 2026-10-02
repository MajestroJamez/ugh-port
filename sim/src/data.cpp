// Loader of the extractor's export "UGHSIM01" (extractor/src/main/kotlin/ugh/extractor/Sim.kt).
#include "sim.hpp"

#include <cstring>
#include <fstream>
#include <iterator>

namespace ugh {

bool Data::load(const std::string& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { error = "cannot open " + path; return false; }
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto u16at = [&](size_t o) { return static_cast<uint32_t>(file[o] | (file[o + 1] << 8)); };
    auto u32at = [&](size_t o) { return u16at(o) | (u16at(o + 2) << 16); };
    if (file.size() < 12 || std::memcmp(file.data(), "UGHSIM01", 8) != 0) { error = path + " is not a UGHSIM01 file"; return false; }
    uint32_t count = u32at(8);
    if (file.size() < 12 + 20ull * count) { error = path + ": truncated block table"; return false; }
    for (uint32_t i = 0; i < count; i++) {
        size_t e = 12 + 20ull * i;
        std::string name(reinterpret_cast<const char*>(&file[e]), strnlen(reinterpret_cast<const char*>(&file[e]), 8));
        uint32_t off = u16at(e + 10), start = u32at(e + 12), length = u32at(e + 16);
        if (start + static_cast<uint64_t>(length) > file.size()) { error = path + ": block " + name + " outside the file"; return false; }
        std::vector<uint8_t> block(file.begin() + start, file.begin() + start + length);
        if (name == "DGROUP") {
            if (off + block.size() > dgroup.size()) { error = "DGROUP block too large"; return false; }
            std::copy(block.begin(), block.end(), dgroup.begin() + off);
        } else if (name == "MAPS") {
            maps = std::move(block);
        } else if (name == "MASK") {
            if (block.size() != MASK_ROW_BYTES * MASK_HEIGHT) { error = "collision mask of an unexpected size"; return false; }
            masks[static_cast<int>(off)] = std::move(block);
        }
    }
    if (masks.empty() || maps.empty()) { error = path + ": no level data"; return false; }
    return true;
}

}  // namespace ugh
