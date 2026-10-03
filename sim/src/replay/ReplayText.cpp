#include "replay/ReplayText.hpp"

#include <cstdio>

namespace ugh::replay {

namespace {

constexpr int NONE_WORD = 0xffff;

}  // namespace

std::string hex(int value) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%04x", value & 0xffff);
    return buf;
}

std::optional<int> parseNumber(const std::string& text, int base) {
    try {
        size_t used = 0;
        long long v = std::stoll(text, &used, base);
        if (used != text.size()) return std::nullopt;
        return static_cast<int>(v);
    } catch (...) {   // not a number, or too big
        return std::nullopt;
    }
}

std::optional<int> parseHex(const std::string& text) {
    if (text.rfind("0x", 0) != 0) return std::nullopt;
    return parseNumber(text.substr(2), 16);
}

std::string formatWord(Format format, core::Word word) {
    switch (format) {
        case Format::Signed: return std::to_string(word.value());
        case Format::Unsigned: return std::to_string(word.bits());
        case Format::Byte: return std::to_string(word.bits() & 0xff);
        case Format::Hex: return hex(word.bits());
        case Format::HexOrNone: return word.bits() == NONE_WORD ? "none" : hex(word.bits());
        case Format::HexOrNoneIfZero: return word.bits() == 0 ? "none" : hex(word.bits());
    }
    return "";
}

std::optional<int> parseWord(Format format, const std::string& text) {
    switch (format) {
        case Format::Signed: case Format::Unsigned: case Format::Byte: return parseNumber(text);
        case Format::Hex: return parseHex(text);
        case Format::HexOrNone: return text == "none" ? std::optional<int>(NONE_WORD) : parseHex(text);
        case Format::HexOrNoneIfZero: return text == "none" ? std::optional<int>(0) : parseHex(text);
    }
    return std::nullopt;
}

}  // namespace ugh::replay
