#include "data/ugd/PlacementReader.hpp"

#include <memory>

#include "data/levels/BlowerPlacement.hpp"
#include "data/levels/FlyerPlacement.hpp"
#include "data/levels/RoutePassengerPlacement.hpp"
#include "data/levels/StandingPassengerPlacement.hpp"
#include "data/levels/TreePlacement.hpp"
#include "data/levels/WalkerPlacement.hpp"

namespace ugh::data::ugd {

using units::Fixed;

namespace {

// a standing passenger placement names no kind: every standing passenger is of the passenger kind of this name
constexpr const char* STANDING_KIND = "standing";

}  // namespace

const RecordTable<PlacementReader>::Entry PlacementReader::PLACEMENTS[] = {
    {"routePassenger", &PlacementReader::readRoutePassenger},
    {"standingPassenger", &PlacementReader::readStandingPassenger},
    {"flyer", &PlacementReader::readFlyer},
    {"walker", &PlacementReader::readWalker},
    {"blower", &PlacementReader::readBlower},
    {"tree", &PlacementReader::readTree}};

bool PlacementReader::reads(const std::string& type) { return RecordTable<PlacementReader>::has(PLACEMENTS, type); }

bool PlacementReader::read(const UgdRecord& r, levels::LevelDefinition& level) {
    level_ = &level;
    return RecordTable<PlacementReader>::read(*this, PLACEMENTS, r);
}

bool PlacementReader::readRoutePassenger(const UgdRecord&) {
    std::string kindName;
    std::vector<levels::Route::Stop> stops;
    if (!in_.only({"kind", "route"}) || !in_.text("kind", kindName)) return false;
    const kinds::RoutePassengerKind* kind = kinds_.routeKind(kindName);
    if (!kind) return in_.fail("no route passenger kind " + kindName);
    if (!readRoute(stops)) return false;
    level_->passengers.push_back(std::make_unique<levels::RoutePassengerPlacement>(*kind, levels::Route{std::move(stops)}));
    return true;
}

bool PlacementReader::readRoute(std::vector<levels::Route::Stop>& stops) {
    std::string route;
    if (!in_.text("route", route)) return false;
    std::vector<std::string> entries = RecordReader::split(route, ',');
    for (size_t e = 0; e < entries.size(); e++) {
        std::vector<std::string> padDelay = RecordReader::split(entries[e], '/');
        int padIndex = 0, delay = 0;
        bool last = e + 1 == entries.size();
        if (padDelay.size() != (last ? 1u : 2u) || !RecordReader::parseInt(padDelay[0], padIndex) ||
            (!last && !RecordReader::parseInt(padDelay[1], delay)) || !pad(padIndex))
            return in_.fail("bad route " + route);
        if (!stops.empty()) stops.back().targetPad = padIndex;
        if (!last) stops.push_back({padIndex, delay, 0});
    }
    return !stops.empty() || in_.fail("a route without a stop");
}

bool PlacementReader::readStandingPassenger(const UgdRecord&) {
    int x = 0, y = 0;
    const kinds::StandingPassengerKind* kind = kinds_.standingKind(STANDING_KIND);
    if (!kind) return in_.fail(std::string("no passenger kind ") + STANDING_KIND);
    if (!in_.only({"x", "y"}) || !in_.number("x", x) || !in_.number("y", y)) return false;
    level_->passengers.push_back(
        std::make_unique<levels::StandingPassengerPlacement>(*kind, Fixed::fromRaw(x), Fixed::fromRaw(y)));
    return true;
}

bool PlacementReader::readFlyer(const UgdRecord&) {
    int startDelay = 0, speed = 0;
    if (!in_.only({"startDelay", "speed"}) || !in_.number("startDelay", startDelay) || !in_.number("speed", speed))
        return false;
    level_->enemies.push_back(std::make_unique<levels::FlyerPlacement>(startDelay, Fixed::fromRaw(speed)));
    return true;
}

bool PlacementReader::readWalker(const UgdRecord&) {
    int padIndex = 0, x = 0, y = 0, speed = 0;
    if (!in_.only({"pad", "x", "y", "speed"}) || !in_.number("pad", padIndex) || !in_.number("x", x) ||
        !in_.number("y", y) || !in_.number("speed", speed) || !pad(padIndex))
        return false;
    level_->enemies.push_back(std::make_unique<levels::WalkerPlacement>(padIndex, Fixed::fromRaw(x), Fixed::fromRaw(y),
                                                                        Fixed::fromRaw(speed)));
    return true;
}

bool PlacementReader::readBlower(const UgdRecord&) {
    int x = 0, y = 0;
    if (!in_.only({"x", "y"}) || !in_.number("x", x) || !in_.number("y", y)) return false;
    level_->enemies.push_back(std::make_unique<levels::BlowerPlacement>(Fixed::fromRaw(x), Fixed::fromRaw(y)));
    return true;
}

/** `drops=<bonus kind>,...` or `drops=-`: one bonus item for each passenger that bounces off the tree. */
bool PlacementReader::readTree(const UgdRecord&) {
    int x = 0, y = 0;
    std::string dropText;
    if (!in_.only({"x", "y", "drops"}) || !in_.number("x", x) || !in_.number("y", y) || !in_.text("drops", dropText))
        return false;
    std::vector<const kinds::BonusKind*> drops;
    if (dropText != "-") {
        for (const std::string& name : RecordReader::split(dropText, ',')) {
            const kinds::BonusKind* drop = kinds_.bonusKind(name);
            if (!drop) return in_.fail("no bonus kind " + name);
            drops.push_back(drop);
        }
    }
    level_->enemies.push_back(std::make_unique<levels::TreePlacement>(Fixed::fromRaw(x), Fixed::fromRaw(y), drops));
    return true;
}

bool PlacementReader::pad(int index) {
    return (index >= 0 && index < static_cast<int>(level_->pads.size())) || in_.fail("no pad " + std::to_string(index));
}

}  // namespace ugh::data::ugd
