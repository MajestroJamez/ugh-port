// Reads the animations and the kinds of the game data.
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "data/GameData.hpp"
#include "data/RecordReader.hpp"
#include "data/UgdRecord.hpp"

namespace ugh::data {

/**
 * Reads the animations and the kinds of passengers, enemies and bonus items into the game data, and finds them by
 * name for the levels and the rules.
 */
class KindsReader {
public:
    KindsReader(RecordReader& in, GameData& data) : in_(in), data_(data) {}

    bool readAnimation(const UgdRecord& r);
    bool readPassengerKind(const UgdRecord& r);
    /** A route kind and its water kind name each other (after all kinds are read). */
    bool linkPassengerKinds(const std::vector<UgdRecord>& records);
    bool readBonusKind(const UgdRecord& r);
    bool readEnemyKind(const UgdRecord& r);

    /** The kinds by name; nullptr when there is none. */
    const RoutePassengerKind* routeKind(const std::string& name) const;
    const StandingPassengerKind* standingKind(const std::string& name) const;
    const BonusKind* bonusKind(const std::string& name) const;

private:
    RecordReader& in_;
    GameData& data_;
    std::map<std::string, const Animation*> animations_;
    std::set<std::string> passengerKindNames_;
    std::map<std::string, RoutePassengerKind*> routeKinds_;
    std::map<std::string, SwimmerKind*> swimmerKinds_;
    std::map<std::string, const StandingPassengerKind*> standingKinds_;
    std::map<std::string, const BonusKind*> bonusKinds_;

    bool animated(const UgdRecord& r, AnimatedPassengerKind& kind);
    static void named(PassengerKind& kind, const std::string& name, const Box& box);
    bool animation(const UgdRecord& r, const char* key, const Animation*& out);
    bool pair(const UgdRecord& r, const char* key, AnimationPair& out);
};

}  // namespace ugh::data
