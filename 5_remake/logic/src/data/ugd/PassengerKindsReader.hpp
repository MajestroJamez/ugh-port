// Reads the kinds of passengers of the game data.
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "data/GameData.hpp"
#include "data/ugd/AnimationsReader.hpp"
#include "data/ugd/RecordReader.hpp"
#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/**
 * Reads the kinds of passengers - `passengerKind <name> type=route|water|standing ...` - and links each route kind
 * with its water kind (they name each other); finds them by name for the levels.
 */
class PassengerKindsReader {
public:
    /** The type of the records it reads. */
    static constexpr const char* RECORD = "passengerKind";

    PassengerKindsReader(RecordReader& in, AnimationsReader& animations, GameData::Contents& data)
        : in_(in), animations_(animations), data_(data) {}

    bool readPassengerKind(const UgdRecord& r);
    /** A route kind and its water kind name each other (after all kinds are read). */
    bool link(const std::vector<UgdRecord>& records);

    /** The kinds by name; nullptr when there is none. */
    const kinds::RoutePassengerKind* routeKind(const std::string& name) const;
    const kinds::StandingPassengerKind* standingKind(const std::string& name) const;

private:
    RecordReader& in_;
    AnimationsReader& animations_;
    GameData::Contents& data_;
    std::set<std::string> names_;
    std::map<std::string, kinds::RoutePassengerKind*> routeKinds_;
    std::map<std::string, kinds::SwimmerKind*> swimmerKinds_;
    std::map<std::string, const kinds::StandingPassengerKind*> standingKinds_;

    bool readRouteKind(const UgdRecord& r, const kinds::Box& box);
    bool readSwimmerKind(const UgdRecord& r, const kinds::Box& box);
    bool readStandingKind(const UgdRecord& r, const kinds::Box& box);
    /** What a passenger with a route shows and pays, on land and in the water. */
    bool readAnimated(kinds::AnimatedPassengerKind& kind);
};

}  // namespace ugh::data::ugd
