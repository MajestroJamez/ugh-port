// Reads the animations and the kinds of the game data.
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "data/GameData.hpp"
#include "data/ugd/AnimationsReader.hpp"
#include "data/ugd/PassengerKindsReader.hpp"
#include "data/ugd/RecordReader.hpp"
#include "data/ugd/RecordTable.hpp"
#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/**
 * Reads the animations and the kinds of passengers, enemies and bonus items into the game data, and finds them by
 * name for the levels and the rules. The passenger kinds have a reader of their own (`PassengerKindsReader`).
 */
class KindsReader {
public:
    KindsReader(RecordReader& in, GameData::Contents& data)
        : in_(in), data_(data), animations_(in, data), passengers_(in, animations_, data) {}

    /** It reads records of `type`. */
    static bool reads(const std::string& type);
    /** All animations first, then the kinds (they name animations), then the passenger kinds are linked. */
    bool readAll(const std::vector<UgdRecord>& records);

    /** The kinds by name; nullptr when there is none. */
    const kinds::RoutePassengerKind* routeKind(const std::string& name) const { return passengers_.routeKind(name); }
    const kinds::StandingPassengerKind* standingKind(const std::string& name) const {
        return passengers_.standingKind(name);
    }
    const kinds::BonusKind* bonusKind(const std::string& name) const;

private:
    /** The kinds by the type of their record (the animations are read before them). */
    static const RecordTable<KindsReader>::Entry KINDS[];

    RecordReader& in_;
    GameData::Contents& data_;
    AnimationsReader animations_;
    PassengerKindsReader passengers_;
    std::map<std::string, const kinds::BonusKind*> bonusKinds_;
    std::set<std::string> enemyKinds_;   // the types of the enemy kinds read so far

    bool readPassengerKind(const UgdRecord& r) { return passengers_.readPassengerKind(r); }
    bool readBonusKind(const UgdRecord& r);
    bool readFlyerKind(const UgdRecord& r);
    bool readWalkerKind(const UgdRecord& r);
    bool readBlowerKind(const UgdRecord& r);
    bool readTreeKind(const UgdRecord& r);
    /** The kind of an enemy of `type` is read the first time (a second one is an error). */
    bool once(const char* type);
};

}  // namespace ugh::data::ugd
