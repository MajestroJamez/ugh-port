// How the replays write numbers.
#pragma once

#include <optional>
#include <string>

#include "core/Word.hpp"

namespace ugh::replay {

/** How a replay writes a word of the original. */
enum class Format {
    Signed,            // -12
    Unsigned,          // 65524
    Byte,              // the low byte: 244
    Hex,               // 0xfff4
    HexOrNone,         // 0xffff is "none"
    HexOrNoneIfZero,   // 0 is "none"
};

/** "0x" and four hex digits of the low 16 bits. */
std::string hex(int value);

/** The whole text as a number in `base`; nothing when it is not one. */
std::optional<int> parseNumber(const std::string& text, int base = 10);

/** "0x" and hex digits. */
std::optional<int> parseHex(const std::string& text);

std::string formatWord(Format format, core::Word word);

/** The number in a text written in `format` (not yet cut to 16 bits); nothing when it is not one. */
std::optional<int> parseWord(Format format, const std::string& text);

}  // namespace ugh::replay
