// Bytes as text that survives a chat or a mail.
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ugh::record {

/**
 * Base64 with the URL's alphabet (RFC 4648 section 5: A-Z a-z 0-9 - _), without padding: 4 characters for 3 bytes,
 * none of them special in a URL, a file name, a chat or a mail. Reading skips white space (a mail breaks long lines).
 */
class Base64Url {
public:
    static std::string encode(std::span<const uint8_t> bytes);
    /** The bytes; none when a character is outside the alphabet or the length cannot be one of bytes. */
    static std::optional<std::vector<uint8_t>> decode(std::string_view text);
};

}  // namespace ugh::record
