// Reads the game data file.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "data/GameData.hpp"

namespace ugh::data {

/**
 * Reads the game data in the format "UGD 1" (assets/logic/ugh-data.ugd, written by the extractor; described in
 * 2_reverse_engineering/notes/phase2-data.md) and checks it, so that the logic needs no checks: the header, known records and keys,
 * names that exist, pad indexes inside their level, routes with a stop, complete masks, levels in both orders.
 */
class DataFileReader {
public:
    /** The data of the file at `path`; nullptr when it cannot be read, `error` says why. */
    static std::unique_ptr<const GameData> read(const std::string& path, std::string& error);

    /** The data from the text of a file (tests). */
    static std::unique_ptr<const GameData> parse(std::string_view text, std::string& error);

private:
    /** One line: `<type> [<name>] <key>=<value> ...`. */
    struct Record {
        int line = 0;
        std::string type, name;
        std::map<std::string, std::string> fields;
    };

    std::unique_ptr<GameData> data_;
    std::string error_;
    const Record* current_ = nullptr;   // the record being read, for the error text

    std::map<std::string, const Animation*> animations_;
    std::map<std::string, PassengerKind*> passengerKinds_;
    std::map<std::string, const BonusKind*> bonusKinds_;
    std::map<int, const LevelDefinition*> levels_;

    DataFileReader();

    bool fail(const std::string& what);
    bool readAll(const std::vector<Record>& records);
    bool readAnimation(const Record& r);
    bool readPassengerKind(const Record& r);
    bool linkPassengerKinds(const std::vector<Record>& records);
    bool readBonusKind(const Record& r);
    bool readEnemyKind(const Record& r);
    bool readRules(const Record& r);
    bool readSprites(const Record& r);
    bool readKey(const Record& r);
    bool readLevel(const std::vector<Record>& records, size_t& i);
    bool readLevelPart(const Record& r, LevelDefinition& level, std::vector<uint8_t>& mask, int& maskRows);
    bool readOrder(const Record& r);

    // values of the current record; false (and the error) when the key is missing or its value is bad
    bool only(const Record& r, std::initializer_list<const char*> keys);
    bool text(const Record& r, const char* key, std::string& out);
    bool number(const Record& r, const char* key, int& out);
    bool numbers(const Record& r, const char* key, size_t count, std::vector<int>& out);
    bool range(const Record& r, const char* key, int& first, int& last);
    bool box(const Record& r, Box& out);
    bool animation(const Record& r, const char* key, const Animation*& out);
    bool pair(const Record& r, const char* key, AnimationPair& out);
    bool pad(const LevelDefinition& level, int index);

    static bool parseInt(const std::string& text, int& out);
    static std::vector<std::string> split(const std::string& text, char separator);
};

}  // namespace ugh::data
