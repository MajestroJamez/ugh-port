// The lines of the game data file.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/**
 * The lines of the game data (format "UGD 1", 2_reverse_engineering/notes/phase2-data.md) as records: the header
 * first, then one record per line; empty lines and `#` comments are skipped.
 */
class UgdTokenizer {
public:
    /** The records of `text`; false (and `error`) when the header is missing or a line is bad. */
    static bool tokenize(std::string_view text, std::vector<UgdRecord>& out, std::string& error);
    /** The records of the file at `path`; false (and `error`, with the path) when it cannot be read or is bad. */
    static bool tokenizeFile(const std::string& path, std::vector<UgdRecord>& out, std::string& error);
};

}  // namespace ugh::data::ugd
