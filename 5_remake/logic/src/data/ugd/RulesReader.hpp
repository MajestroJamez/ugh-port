// Reads the rules and the sprites of the game data.
#pragma once

#include <string>
#include <vector>

#include "data/GameData.hpp"
#include "data/ugd/KindsReader.hpp"
#include "data/ugd/RecordReader.hpp"
#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/**
 * Reads the rules (crash and multiplier limits, the bonus of a quick delivery) and the sprite numbers the logic
 * names; both must be there.
 */
class RulesReader {
public:
    RulesReader(RecordReader& in, const KindsReader& kinds, GameData::Contents& data)
        : in_(in), kinds_(kinds), data_(data) {}

    /** It reads records of `type`. */
    static bool reads(const std::string& type);
    bool readAll(const std::vector<UgdRecord>& records);

private:
    RecordReader& in_;
    const KindsReader& kinds_;
    GameData::Contents& data_;

    bool readRules();
    bool readSprites();
};

}  // namespace ugh::data::ugd
