#include "data/DataFileReader.hpp"

#include "data/KindsReader.hpp"
#include "data/LevelReader.hpp"
#include "data/RecordReader.hpp"
#include "data/RulesReader.hpp"
#include "data/UgdTokenizer.hpp"

namespace ugh::data {

namespace {

// the `key` records are the keys of the PC keyboard: not for the logic (the replays' keyboard in 6_verification)
constexpr const char* KNOWN[] = {"rules", "sprites", "key", "animation", "passengerKind", "flyerKind", "walkerKind",
                                 "blowerKind", "treeKind", "bonusKind", "level", "pad", "routePassenger",
                                 "standingPassenger", "flyer", "walker", "blower", "tree", "mask", "order"};

bool isEnemyKind(const UgdRecord& r) {
    return r.type == "flyerKind" || r.type == "walkerKind" || r.type == "blowerKind" || r.type == "treeKind";
}

bool isLevelPart(const UgdRecord& r) {
    return r.type == "pad" || r.type == "routePassenger" || r.type == "standingPassenger" || r.type == "flyer" ||
           r.type == "walker" || r.type == "blower" || r.type == "tree" || r.type == "mask";
}

}  // namespace

std::unique_ptr<const GameData> DataFileReader::read(const std::string& path, std::string& error) {
    std::vector<UgdRecord> records;
    if (!UgdTokenizer::tokenizeFile(path, records, error)) return nullptr;
    auto data = build(records, error);
    if (!data) error = path + ": " + error;
    return data;
}

std::unique_ptr<const GameData> DataFileReader::parse(std::string_view text, std::string& error) {
    std::vector<UgdRecord> records;
    if (!UgdTokenizer::tokenize(text, records, error)) return nullptr;
    return build(records, error);
}

std::unique_ptr<const GameData> DataFileReader::build(const std::vector<UgdRecord>& records, std::string& error) {
    std::unique_ptr<GameData> data(new GameData());
    RecordReader in;
    if (!readAll(records, in, *data)) {
        error = in.error();
        return nullptr;
    }
    return data;
}

bool DataFileReader::readAll(const std::vector<UgdRecord>& records, RecordReader& in, GameData& data) {
    for (const UgdRecord& r : records) {
        in.at(&r);
        bool known = false;
        for (const char* type : KNOWN) known = known || r.type == type;
        if (!known) return in.fail("unknown record");
    }
    KindsReader kinds(in, data);
    RulesReader rules(in, kinds, data);
    LevelReader levels(in, kinds, data);
    if (!readKinds(records, in, kinds) || !readRest(records, in, rules, levels)) return false;
    in.at(nullptr);
    if (data.order_[0].empty() || data.order_[1].empty()) return in.fail("a level order missing");
    return true;
}

bool DataFileReader::readKinds(const std::vector<UgdRecord>& records, RecordReader& in, KindsReader& kinds) {
    for (const UgdRecord& r : records) {
        in.at(&r);
        if (r.type == "animation" && !kinds.readAnimation(r)) return false;
    }
    for (const UgdRecord& r : records) {
        in.at(&r);
        if (r.type == "bonusKind" && !kinds.readBonusKind(r)) return false;
        if (r.type == "passengerKind" && !kinds.readPassengerKind(r)) return false;
        if (isEnemyKind(r) && !kinds.readEnemyKind(r)) return false;
    }
    return kinds.linkPassengerKinds(records);
}

bool DataFileReader::readRest(const std::vector<UgdRecord>& records, RecordReader& in, RulesReader& rules,
                              LevelReader& levels) {
    bool rulesRead = false, spritesRead = false;
    for (size_t i = 0; i < records.size(); i++) {
        const UgdRecord& r = records[i];
        in.at(&r);
        if (r.type == "rules") {
            if (!rules.readRules(r)) return false;
            rulesRead = true;
        } else if (r.type == "sprites") {
            if (!rules.readSprites(r)) return false;
            spritesRead = true;
        } else if (r.type == "level") {
            if (!levels.readLevel(records, i)) return false;
        } else if (r.type == "order") {
            if (!levels.readOrder(r)) return false;
        } else if (isLevelPart(r)) {
            return in.fail("outside a level");
        }
    }
    in.at(nullptr);
    return (rulesRead && spritesRead) || in.fail("rules or sprites missing");
}

}  // namespace ugh::data
