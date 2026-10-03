// Reads the game data file.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "data/GameData.hpp"
#include "data/UgdRecord.hpp"

namespace ugh::data {

class KindsReader;
class LevelReader;
class RecordReader;
class RulesReader;

/**
 * Reads the game data in the format "UGD 1" (assets/logic/ugh-data.ugd, written by the extractor; described in
 * 2_reverse_engineering/notes/phase2-data.md) and checks it, so that the logic needs no checks: the header, known
 * records and keys, names that exist, pad indexes inside their level, routes with a stop, complete masks, levels in
 * both orders. It puts the parts together: UgdTokenizer (lines to records), RecordReader (values and errors),
 * KindsReader, RulesReader and LevelReader.
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
    /** The records in the order their references need: animations, kinds, rules, levels, orders. */
    static bool readAll(const std::vector<UgdRecord>& records, RecordReader& in, GameData& data);
    static bool readKinds(const std::vector<UgdRecord>& records, RecordReader& in, KindsReader& kinds);
    static bool readRest(const std::vector<UgdRecord>& records, RecordReader& in, RulesReader& rules,
                         LevelReader& levels);
};

}  // namespace ugh::data
