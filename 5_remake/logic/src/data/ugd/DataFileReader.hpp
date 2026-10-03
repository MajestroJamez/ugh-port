// Reads the game data file.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "data/GameData.hpp"
#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

class RecordReader;

/**
 * Reads the game data in the format "UGD 1" (assets/logic/ugh-data.ugd, written by the extractor; described in
 * 2_reverse_engineering/notes/phase2-data.md) and checks it, so that the logic needs no checks: the header, known
 * records and keys, names that exist, pad indexes inside their level, routes with a stop, complete masks, levels in
 * both orders. It puts the parts together: UgdTokenizer (lines to records), RecordReader (values and errors), and the
 * readers of the parts, each with the record types it reads: KindsReader, RulesReader and LevelReader. The kinds of
 * the enemies, the rules and the sprites must be there exactly once; a level with a standing passenger needs the
 * passenger kind "standing" (its placement names no kind).
 */
class DataFileReader {
public:
    /** The data of the file at `path`; nullptr when it cannot be read, `error` says why. */
    static std::unique_ptr<const GameData> read(const std::string& path, std::string& error);

    /** The data from the text of a file (tests). */
    static std::unique_ptr<const GameData> parse(std::string_view text, std::string& error);

private:
    /** The game data of the records, checked; nullptr and `error` when they are bad. */
    static std::unique_ptr<const GameData> build(const std::vector<UgdRecord>& records, std::string& error);
    static bool readAll(const std::vector<UgdRecord>& records, RecordReader& in, GameData::Contents& contents);
};

}  // namespace ugh::data::ugd
