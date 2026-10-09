#include "record/Base64Url.hpp"

namespace ugh::record {

namespace {

constexpr std::string_view ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

int valueOf(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

bool space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

}  // namespace

std::string Base64Url::encode(std::span<const uint8_t> bytes) {
    std::string text;
    text.reserve((bytes.size() * 4 + 2) / 3);
    uint32_t bits = 0;
    int count = 0;
    for (uint8_t b : bytes) {
        bits = bits << 8 | b;
        count += 8;
        while (count >= 6) {
            count -= 6;
            text.push_back(ALPHABET[(bits >> count) & 63]);
        }
    }
    if (count > 0) text.push_back(ALPHABET[(bits << (6 - count)) & 63]);
    return text;
}

std::optional<std::vector<uint8_t>> Base64Url::decode(std::string_view text) {
    std::vector<uint8_t> bytes;
    uint32_t bits = 0;
    int count = 0, characters = 0;
    for (char c : text) {
        if (space(c)) continue;
        int value = valueOf(c);
        if (value < 0) return std::nullopt;
        characters++;
        bits = bits << 6 | static_cast<uint32_t>(value);
        count += 6;
        if (count >= 8) {
            count -= 8;
            bytes.push_back(static_cast<uint8_t>(bits >> count));
        }
    }
    // a last group of 1 character holds no byte; the bits left over must be 0 (one text a byte string)
    if (characters % 4 == 1 || (bits & ((1u << count) - 1)) != 0) return std::nullopt;
    return bytes;
}

}  // namespace ugh::record
