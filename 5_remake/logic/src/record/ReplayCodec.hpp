// The binary .ughr file.
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "record/Recording.hpp"

namespace ugh::record {

/**
 * A recording as the bytes of a `.ughr` file (docs/replay-format.md): "UGHR", the format, the logic and the data it
 * was made with, what its first attempt started from, how it went, its label, the inputs as one small number each
 * (the steps since the input before - the run of steps without a change of the keys - and what changed), a CRC-32 of
 * all that. Reading checks everything: what is not a replay, a newer format, a damaged one are refused with the reason.
 */
class ReplayCodec {
public:
    static constexpr std::string_view MAGIC = "UGHR";
    static constexpr int FORMAT = 1;
    static constexpr int MAX_STEPS = 2520000;   // ten hours of the original's frames
    static constexpr int MAX_INPUTS = 1000000;

    static std::vector<uint8_t> write(const Recording& recording);
    /** The recording of `bytes`; none, and why in `error`, when they are not a replay this format reads. */
    static std::optional<Recording> read(std::span<const uint8_t> bytes, std::string& error);

    /** The reasons a replay is refused. */
    static constexpr const char* NOT_A_REPLAY = "not a UGH! replay";
    static constexpr const char* NEWER = "a replay of a newer format: a newer version of the game made it";
    static constexpr const char* DAMAGED_CHECKSUM = "damaged: its checksum does not match";
    static constexpr const char* DAMAGED = "damaged: it cannot be read";
};

}  // namespace ugh::record
