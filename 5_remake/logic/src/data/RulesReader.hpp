// Reads the rules and the sprites of the game data.
#pragma once

#include "data/GameData.hpp"
#include "data/KindsReader.hpp"
#include "data/RecordReader.hpp"
#include "data/UgdRecord.hpp"

namespace ugh::data {

/** Reads the rules (crash and multiplier limits, the bonus of a quick delivery) and the sprite numbers the logic names. */
class RulesReader {
public:
    RulesReader(RecordReader& in, const KindsReader& kinds, GameData& data) : in_(in), kinds_(kinds), data_(data) {}

    bool readRules(const UgdRecord& r);
    bool readSprites(const UgdRecord& r);

private:
    RecordReader& in_;
    const KindsReader& kinds_;
    GameData& data_;
};

}  // namespace ugh::data
