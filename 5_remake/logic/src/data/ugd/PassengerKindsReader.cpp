#include "data/ugd/PassengerKindsReader.hpp"

#include <memory>

namespace ugh::data::ugd {

bool PassengerKindsReader::readPassengerKind(const UgdRecord& r) {
    std::string type;
    kinds::Box box;
    if (r.name.empty() || names_.count(r.name)) return in_.fail("a passenger kind without a name or twice");
    if (!in_.text("type", type) || !in_.box(box)) return false;
    names_.insert(r.name);
    if (type == "route") return readRouteKind(r, box);
    if (type == "water") return readSwimmerKind(r, box);
    if (type == "standing") return readStandingKind(r, box);
    return in_.fail("unknown passenger type " + type);
}

bool PassengerKindsReader::readRouteKind(const UgdRecord& r, const kinds::Box& box) {
    auto kind = std::make_unique<kinds::RoutePassengerKind>();
    if (!in_.only({"type", "box", "standing", "waving", "walk", "comingOut", "goingIn", "animDelay", "fare", "fareMin",
                   "look", "waterKind"}) ||
        !animations_.animation("comingOut", kind->comingOut) || !animations_.animation("goingIn", kind->goingIn) ||
        !readAnimated(*kind) || !in_.number("look", kind->look))
        return false;
    kind->name = r.name;
    kind->box = box;
    routeKinds_[r.name] = kind.get();
    data_.routePassengerKinds.push_back(std::move(kind));
    return true;
}

bool PassengerKindsReader::readSwimmerKind(const UgdRecord& r, const kinds::Box& box) {
    auto kind = std::make_unique<kinds::SwimmerKind>();
    int rescuable = 0;
    if (!in_.only({"type", "box", "standing", "waving", "walk", "animDelay", "fare", "fareMin", "swimTime", "landKind",
                   "rescuable"}) ||
        !in_.number("swimTime", kind->swimTime) || !in_.number("rescuable", rescuable) || !readAnimated(*kind))
        return false;
    kind->rescuable = rescuable != 0;
    kind->name = r.name;
    kind->box = box;
    swimmerKinds_[r.name] = kind.get();
    data_.swimmerKinds.push_back(std::move(kind));
    return true;
}

bool PassengerKindsReader::readStandingKind(const UgdRecord& r, const kinds::Box& box) {
    auto kind = std::make_unique<kinds::StandingPassengerKind>();
    if (!in_.only({"type", "box", "look"}) || !in_.number("look", kind->look)) return false;
    kind->name = r.name;
    kind->box = box;
    standingKinds_[r.name] = kind.get();
    data_.standingPassengerKinds.push_back(std::move(kind));
    return true;
}

bool PassengerKindsReader::readAnimated(kinds::AnimatedPassengerKind& kind) {
    return animations_.animation("standing", kind.standing) && animations_.animation("waving", kind.waving) &&
           animations_.pair("walk", kind.walking) && in_.number("animDelay", kind.animDelay) &&
           in_.number("fare", kind.fare) && in_.number("fareMin", kind.fareMin);
}

bool PassengerKindsReader::link(const std::vector<UgdRecord>& records) {
    for (const UgdRecord& r : records) {
        if (r.type != "passengerKind") continue;
        in_.at(&r);
        auto route = routeKinds_.find(r.name);
        auto swimmer = swimmerKinds_.find(r.name);
        if (route == routeKinds_.end() && swimmer == swimmerKinds_.end()) continue;
        const char* key = route != routeKinds_.end() ? "waterKind" : "landKind";
        std::string other;
        if (!in_.text(key, other)) return false;
        if (!names_.count(other)) return in_.fail("no passenger kind " + other);
        if (route != routeKinds_.end()) {
            auto it = swimmerKinds_.find(other);
            if (it == swimmerKinds_.end()) return in_.fail(std::string(key) + " " + other + " is of the wrong type");
            route->second->swimmer = it->second;
        } else {
            auto it = routeKinds_.find(other);
            if (it == routeKinds_.end()) return in_.fail(std::string(key) + " " + other + " is of the wrong type");
            swimmer->second->land = it->second;
        }
    }
    return true;
}

const kinds::RoutePassengerKind* PassengerKindsReader::routeKind(const std::string& name) const {
    auto it = routeKinds_.find(name);
    return it == routeKinds_.end() ? nullptr : it->second;
}

const kinds::StandingPassengerKind* PassengerKindsReader::standingKind(const std::string& name) const {
    auto it = standingKinds_.find(name);
    return it == standingKinds_.end() ? nullptr : it->second;
}

}  // namespace ugh::data::ugd
