// Reads the levels of the game data.
#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "data/GameData.hpp"
#include "data/KindsReader.hpp"
#include "data/RecordReader.hpp"
#include "data/UgdRecord.hpp"

namespace ugh::data {

/**
 * Reads the levels - a level record and its pads, passengers, enemies and mask rows after it - and the orders of the
 * levels in both modes; checks the pad numbers, the routes and complete masks.
 */
class LevelReader {
public:
    LevelReader(RecordReader& in, const KindsReader& kinds, GameData& data) : in_(in), kinds_(kinds), data_(data) {}

    /** A level record and the records after it up to the next level or order (`i` ends at its last). */
    bool readLevel(const std::vector<UgdRecord>& records, size_t& i);
    bool readOrder(const UgdRecord& r);

private:
    RecordReader& in_;
    const KindsReader& kinds_;
    GameData& data_;
    std::map<int, const LevelDefinition*> levels_;

    bool readLevelPart(const UgdRecord& r, LevelDefinition& level, std::vector<uint8_t>& mask, int& maskRows);
    /** A pad of the level; false (an error) when there is none. */
    bool pad(const LevelDefinition& level, int index);
};

}  // namespace ugh::data
