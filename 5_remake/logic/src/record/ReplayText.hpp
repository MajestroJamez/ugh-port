// A replay as a line of text to copy and paste.
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "record/Recording.hpp"

namespace ugh::record {

/**
 * A replay as one line of text to copy and paste (a chat, a mail): "UGHR1:" (the format) and the bytes of its file
 * (ReplayCodec) in Base64Url - their CRC-32 checks it. Reading takes either: the text (white space around it and in it
 * skipped, a mail breaks long lines) or the bytes of a file (binary, or the text saved as a file).
 */
class ReplayText {
public:
    static std::string write(const Recording& recording);
    /** The recording of `given` (a text or a file's bytes); none, and why in `error`, when it is not one to play. */
    static std::optional<Recording> read(std::span<const uint8_t> given, std::string& error);

    static constexpr const char* NOT_TEXT = "damaged: its text has a character that does not belong there";
};

}  // namespace ugh::record
