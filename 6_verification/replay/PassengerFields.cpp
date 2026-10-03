#include "replay/PassengerFields.hpp"

#include <optional>
#include <string>

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/standing/StandingPassenger.hpp"
#include "replay/FieldRules.hpp"
#include "replay/FieldTable.hpp"

namespace ugh::replay {

namespace {

using passengers::Passenger;
using passengers::route::RoutePassenger;
using passengers::standing::StandingPassenger;

// ------------------------------------------------------------ which fields a passenger has in which state

const char* const ROUTE_ANIMATED = "ComingOut Waiting Calling Impatient Boarding WalkingToDoor GoingIn Splash Swimming "
                                   "SwimCalling SwimWaving SwimBoarding Sinking";

const FieldRules& routeRules() {
    static const FieldRules rules({
        {"NextStop BehindDoor ComingOut Waiting Calling Impatient Boarding Riding WalkingToDoor GoingIn Gone Splash "
         "Swimming SwimCalling SwimWaving SwimBoarding Sinking",
         "kind state sprite bubble"},
        {ROUTE_ANIMATED, "x y animFrame animDelay seenX seenY"},
        {"NextStop BehindDoor ComingOut Waiting Calling Impatient Boarding Riding WalkingToDoor GoingIn Splash Swimming "
         "SwimCalling SwimWaving SwimBoarding Sinking",
         "routeStop"},
        {"BehindDoor ComingOut Waiting Calling Impatient Boarding Riding WalkingToDoor GoingIn Splash Swimming "
         "SwimCalling SwimWaving SwimBoarding Sinking",
         "pickupPad targetPad"},
        {"Riding", "seenX seenY carrier quickDeliveryTime"},
        {"BehindDoor", "arrivalDelay"},
        {"Calling Impatient SwimCalling SwimWaving", "callTime"},
        {"Waiting", "waitingSpot"},
        {"Splash Sinking", "swimSpeed"},
        {"Swimming", "swimTime"},
    });
    return rules;
}

const FieldRules& standingRules() {
    static const FieldRules rules({
        {"Placed Standing Hanging Falling Gone", "kind state sprite bubble"},
        {"Placed Standing Falling", "x y"},
        {"Hanging", "carrier"},
        {"Falling", "dropSpeedX fallSpeed"},
    });
    return rules;
}

// ------------------------------------------------------------ the values of the fields

std::string orNone(std::optional<int> value) { return value ? std::to_string(*value) : "none"; }

std::string playerOf(const world::Copter* copter) { return copter ? std::to_string(copter->player()) : "none"; }

const char* spotName(passengers::route::WaitingSpot spot) {
    switch (spot) {
        case passengers::route::WaitingSpot::Starting: return "starting";
        case passengers::route::WaitingSpot::Walking: return "walking";
        case passengers::route::WaitingSpot::Reached: return "reached";
    }
    return "";
}

const FieldTable<Passenger>& commonValues() {
    static const FieldTable<Passenger> values{
        {"sprite", [](const Passenger& p) { return orNone(p.sprite()); }},
        {"bubble", [](const Passenger& p) { return orNone(p.bubble()); }},
        {"x", [](const Passenger& p) { return std::to_string(p.x().raw()); }},
        {"y", [](const Passenger& p) { return std::to_string(p.y().raw()); }},
        {"animFrame", [](const Passenger& p) { return std::to_string(p.animator().frame()); }},
        {"animDelay", [](const Passenger& p) { return std::to_string(p.animator().delay()); }},
    };
    return values;
}

const FieldTable<RoutePassenger>& routeValues() {
    static const FieldTable<RoutePassenger> values{
        {"kind", [](const RoutePassenger& p) { return p.kind().name; }},
        {"state", [](const RoutePassenger& p) { return std::string(p.state().name()); }},
        {"routeStop", [](const RoutePassenger& p) { return std::to_string(p.route().stop()); }},
        {"pickupPad", [](const RoutePassenger& p) { return std::to_string(p.route().pickupPad().index()); }},
        {"targetPad", [](const RoutePassenger& p) { return std::to_string(p.route().targetPad().index()); }},
        {"seenX", [](const RoutePassenger& p) { return std::to_string(p.seenX()); }},
        {"seenY", [](const RoutePassenger& p) { return std::to_string(p.seenY()); }},
        {"arrivalDelay", [](const RoutePassenger& p) { return std::to_string(p.route().arrivalDelay()); }},
        {"callTime", [](const RoutePassenger& p) { return std::to_string(p.call().time()); }},
        {"waitingSpot", [](const RoutePassenger& p) { return std::string(spotName(p.call().spot())); }},
        {"carrier", [](const RoutePassenger& p) { return playerOf(p.ride().carrier()); }},
        {"quickDeliveryTime", [](const RoutePassenger& p) { return std::to_string(p.ride().quickDeliveryTime()); }},
        {"swimSpeed", [](const RoutePassenger& p) { return std::to_string(p.swim().speed().raw()); }},
        {"swimTime", [](const RoutePassenger& p) { return std::to_string(p.swim().afloatTime()); }},
    };
    return values;
}

const FieldTable<StandingPassenger>& standingValues() {
    static const FieldTable<StandingPassenger> values{
        {"kind", [](const StandingPassenger& p) { return p.kind().name; }},
        {"state", [](const StandingPassenger& p) { return std::string(p.state().name()); }},
        {"carrier", [](const StandingPassenger& p) { return playerOf(p.carrier()); }},
        {"dropSpeedX", [](const StandingPassenger& p) { return std::to_string(p.dropSpeedX().raw()); }},
        {"fallSpeed", [](const StandingPassenger& p) { return std::to_string(p.fallSpeed().raw()); }},
    };
    return values;
}

std::string prefix(const Passenger& passenger) { return "passenger." + std::to_string(passenger.index()) + "."; }

}  // namespace

void PassengerFields::visit(const RoutePassenger& p) {
    writeFields(p, prefix(p), routeRules(), routeValues(), commonValues(), fields_);
}

void PassengerFields::visit(const StandingPassenger& p) {
    writeFields(p, prefix(p), standingRules(), standingValues(), commonValues(), fields_);
}

}  // namespace ugh::replay
