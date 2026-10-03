#include "data/ugd/DataFileReader.hpp"

#include "data/ugd/KindsReader.hpp"
#include "data/ugd/LevelReader.hpp"
#include "data/ugd/RecordReader.hpp"
#include "data/ugd/RulesReader.hpp"
#include "data/ugd/UgdTokenizer.hpp"

namespace ugh::data::ugd {

namespace {

// the keys of the PC keyboard: not for the logic (the replays' keyboard in 6_verification reads them)
constexpr const char* KEYBOARD = "key";

bool known(const std::string& type) {
    return KindsReader::reads(type) || RulesReader::reads(type) || LevelReader::reads(type) || type == KEYBOARD;
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
    GameData::Contents contents;
    RecordReader in;
    if (!readAll(records, in, contents)) {
        error = in.error();
        return nullptr;
    }
    return std::make_unique<const GameData>(std::move(contents));
}

/** The kinds first (the rules and the levels name them), then the rules, then the levels and their orders. */
bool DataFileReader::readAll(const std::vector<UgdRecord>& records, RecordReader& in, GameData::Contents& contents) {
    for (const UgdRecord& r : records) {
        in.at(&r);
        if (!known(r.type)) return in.fail("unknown record");
    }
    KindsReader kinds(in, contents);
    RulesReader rules(in, kinds, contents);
    LevelReader levels(in, kinds, contents);
    if (!kinds.readAll(records) || !rules.readAll(records) || !levels.readAll(records)) return false;
    in.at(nullptr);
    return (!contents.order[0].empty() && !contents.order[1].empty()) || in.fail("a level order missing");
}

}  // namespace ugh::data::ugd
