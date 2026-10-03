// Reads the levels of the game data.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "data/GameData.hpp"
#include "data/ugd/KindsReader.hpp"
#include "data/ugd/PlacementReader.hpp"
#include "data/ugd/RecordReader.hpp"
#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/**
 * Reads the levels - a level record and its parts after it: pads, mask rows, and the placements of passengers and
 * enemies (`PlacementReader`) - and the orders of the levels in both modes; checks complete masks.
 */
class LevelReader {
public:
    LevelReader(RecordReader& in, const KindsReader& kinds, GameData::Contents& data)
        : in_(in), data_(data), placements_(in, kinds) {}

    /** It reads records of `type`. */
    static bool reads(const std::string& type);
    /** Every level with its parts (up to the next level or order) and the orders; a part outside a level is an error. */
    bool readAll(const std::vector<UgdRecord>& records);

private:
    RecordReader& in_;
    GameData::Contents& data_;
    PlacementReader placements_;
    std::map<int, const levels::LevelDefinition*> byId_;
    // the level being read: its record, what is read of it, its mask rows so far
    const UgdRecord* levelRecord_ = nullptr;
    std::unique_ptr<levels::LevelDefinition> level_;
    std::vector<uint8_t> mask_;
    int maskRows_ = 0;

    bool startLevel(const UgdRecord& r);
    /** A part of the level being read: a pad, a mask row or a placement. */
    bool readPart(const UgdRecord& r);
    /** The level is complete (its mask too): into the data. */
    bool finishLevel();
    bool readOrder(const UgdRecord& r);
    bool readPad();
    bool readMaskRow(const UgdRecord& r);
};

}  // namespace ugh::data::ugd
