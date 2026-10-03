#include "replay/PassengerFields.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/standing/StandingPassenger.hpp"
#include "replay/FieldRules.hpp"

namespace ugh::replay {

namespace {

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

std::string sprite(std::optional<int> s) { return s ? std::to_string(*s) : "none"; }

const char* spotName(passengers::route::WaitingSpot spot) {
    switch (spot) {
        case passengers::route::WaitingSpot::Starting: return "starting";
        case passengers::route::WaitingSpot::Walking: return "walking";
        case passengers::route::WaitingSpot::Reached: return "reached";
    }
    return "";
}

}  // namespace

void PassengerFields::common(const passengers::Passenger& p, const std::string& c, const std::string& field) {
    if (field == "sprite") fields_[c + field] = sprite(p.sprite());
    else if (field == "bubble") fields_[c + field] = sprite(p.bubble());
    else if (field == "x") fields_[c + field] = std::to_string(p.x().raw());
    else if (field == "y") fields_[c + field] = std::to_string(p.y().raw());
    else if (field == "animFrame") fields_[c + field] = std::to_string(p.animator().frame());
    else if (field == "animDelay") fields_[c + field] = std::to_string(p.animator().delay());
}

void PassengerFields::visit(const passengers::route::RoutePassenger& p) {
    std::string c = "passenger." + std::to_string(p.index()) + ".";
    for (const std::string& field : routeRules().fieldsOf(p.state().name())) {
        std::string& v = fields_[c + field];
        if (field == "kind") v = p.kind().name;
        else if (field == "state") v = p.state().name();
        else if (field == "routeStop") v = std::to_string(p.route().stop());
        else if (field == "pickupPad") v = std::to_string(p.route().pickupPad());
        else if (field == "targetPad") v = std::to_string(p.route().targetPad());
        else if (field == "seenX") v = std::to_string(p.seenX());
        else if (field == "seenY") v = std::to_string(p.seenY());
        else if (field == "arrivalDelay") v = std::to_string(p.route().arrivalDelay());
        else if (field == "callTime") v = std::to_string(p.call().time());
        else if (field == "waitingSpot") v = spotName(p.call().spot());
        else if (field == "carrier") v = p.ride().carrier() ? std::to_string(*p.ride().carrier()) : "none";
        else if (field == "quickDeliveryTime") v = std::to_string(p.ride().quickDeliveryTime());
        else if (field == "swimSpeed") v = std::to_string(p.swim().speed().raw());
        else if (field == "swimTime") v = std::to_string(p.swim().afloatTime());
        else common(p, c, field);
    }
}

void PassengerFields::visit(const passengers::standing::StandingPassenger& p) {
    std::string c = "passenger." + std::to_string(p.index()) + ".";
    for (const std::string& field : standingRules().fieldsOf(p.state().name())) {
        std::string& v = fields_[c + field];
        if (field == "kind") v = p.kind().name;
        else if (field == "state") v = p.state().name();
        else if (field == "carrier") v = p.carrier() ? std::to_string(*p.carrier()) : "none";
        else if (field == "dropSpeedX") v = std::to_string(p.dropSpeedX().raw());
        else if (field == "fallSpeed") v = std::to_string(p.fallSpeed());
        else common(p, c, field);
    }
}

}  // namespace ugh::replay
