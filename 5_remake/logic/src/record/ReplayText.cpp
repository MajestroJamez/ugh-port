#include "record/ReplayText.hpp"

#include <string_view>

#include "record/Base64Url.hpp"
#include "record/ReplayCodec.hpp"

namespace ugh::record {

namespace {

bool space(uint8_t c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

/** The bytes of a file start so (the format a small number, below any character of a text). */
bool binary(std::span<const uint8_t> given) {
    const size_t head = ReplayCodec::MAGIC.size();
    return given.size() > head &&
           std::string_view(reinterpret_cast<const char*>(given.data()), head) == ReplayCodec::MAGIC &&
           given[head] < 0x20;
}

}  // namespace

std::string ReplayText::write(const Recording& recording) {
    return std::string(ReplayCodec::MAGIC) + std::to_string(ReplayCodec::FORMAT) + ":" +
           Base64Url::encode(ReplayCodec::write(recording));
}

std::optional<Recording> ReplayText::read(std::span<const uint8_t> given, std::string& error) {
    if (binary(given)) return ReplayCodec::read(given, error);
    // the text: white space and a byte order mark of UTF-8 around it skipped
    size_t begin = 0, end = given.size();
    if (end >= 3 && given[0] == 0xEF && given[1] == 0xBB && given[2] == 0xBF) begin = 3;
    while (begin < end && space(given[begin])) begin++;
    while (end > begin && space(given[end - 1])) end--;
    std::string_view text(reinterpret_cast<const char*>(given.data()) + begin, end - begin);
    const size_t head = ReplayCodec::MAGIC.size();
    const size_t colon = text.find(':');
    int format = 0;
    bool digits = colon != std::string_view::npos && colon > head && colon <= head + 3;
    for (size_t i = head; digits && i < colon; i++) {
        digits = text[i] >= '0' && text[i] <= '9';
        format = format * 10 + (text[i] - '0');
    }
    if (text.substr(0, head) != ReplayCodec::MAGIC || !digits) {
        error = ReplayCodec::NOT_A_REPLAY;
        return std::nullopt;
    }
    if (format > ReplayCodec::FORMAT) {
        error = ReplayCodec::NEWER;
        return std::nullopt;
    }
    std::optional<std::vector<uint8_t>> bytes = Base64Url::decode(text.substr(colon + 1));
    if (!bytes) {
        error = NOT_TEXT;
        return std::nullopt;
    }
    std::optional<Recording> recording = ReplayCodec::read(*bytes, error);
    // the text's format is its file's
    if (recording && format != ReplayCodec::FORMAT) {
        error = ReplayCodec::DAMAGED;
        return std::nullopt;
    }
    if (!recording && error == ReplayCodec::NOT_A_REPLAY) error = ReplayCodec::DAMAGED;
    return recording;
}

}  // namespace ugh::record
