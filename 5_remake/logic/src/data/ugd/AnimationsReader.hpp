// Reads the animations of the game data.
#pragma once

#include <map>
#include <string>

#include "data/GameData.hpp"
#include "data/kinds/Animation.hpp"
#include "data/kinds/AnimationPair.hpp"
#include "data/ugd/RecordReader.hpp"
#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/** Reads the animations and finds them by name for the kinds that show them. */
class AnimationsReader {
public:
    AnimationsReader(RecordReader& in, GameData::Contents& data) : in_(in), data_(data) {}

    /** `animation <name> frames=<sprite>,...`. */
    bool readAnimation(const UgdRecord& r);

    /** The animation the value of `key` names. */
    bool animation(const char* key, const kinds::Animation*& out);
    /** The left and the right animation the value of `key` names (`<left>,<right>`). */
    bool pair(const char* key, kinds::AnimationPair& out);

private:
    RecordReader& in_;
    GameData::Contents& data_;
    std::map<std::string, const kinds::Animation*> byName_;

    bool find(const std::string& name, const kinds::Animation*& out);
};

}  // namespace ugh::data::ugd
