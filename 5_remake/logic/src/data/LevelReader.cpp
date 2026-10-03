#include "data/LevelReader.hpp"

#include <charconv>
#include <memory>
#include <string>

#include "data/BlowerPlacement.hpp"
#include "data/FlyerPlacement.hpp"
#include "data/RoutePassengerPlacement.hpp"
#include "data/StandingPassengerPlacement.hpp"
#include "data/TreePlacement.hpp"
#include "data/WalkerPlacement.hpp"

namespace ugh::data {

namespace {

constexpr int MASK_ROW_DIGITS = CollisionMask::WIDTH / 4;

}  // namespace

bool LevelReader::readLevel(const std::vector<UgdRecord>& records, size_t& i) {
    const UgdRecord& r = records[i];
    auto level = std::make_unique<LevelDefinition>();
    std::string wind;
    std::vector<int> start0, start1;
    int water = 0, waterSpeed = 0;
    if (!RecordReader::parseInt(r.name, level->id)) return in_.fail("a level without a number");
    if (levels_.count(level->id)) return in_.fail("level " + r.name + " twice");
    if (!in_.only(r, {"toDeliver", "wind", "start0", "start1", "water", "waterSpeed"}) ||
        !in_.number(r, "toDeliver", level->toDeliver) || !in_.text(r, "wind", wind) ||
        !in_.numbers(r, "start0", 2, start0) || !in_.numbers(r, "start1", 2, start1) || !in_.number(r, "water", water) ||
        !in_.number(r, "waterSpeed", waterSpeed))
        return false;
    if (wind == "none") level->wind = Wind::None;
    else if (wind == "left") level->wind = Wind::Left;
    else if (wind == "right") level->wind = Wind::Right;
    else return in_.fail("unknown wind " + wind);
    level->startX = {units::Fixed::fromRaw(start0[0]), units::Fixed::fromRaw(start1[0])};
    level->startY = {units::Fixed::fromRaw(start0[1]), units::Fixed::fromRaw(start1[1])};
    level->water = units::Fixed::fromRaw(water);
    level->waterSpeed = units::Fixed::fromRaw(waterSpeed);
    std::vector<uint8_t> mask;
    int maskRows = 0;
    while (i + 1 < records.size() && records[i + 1].type != "level" && records[i + 1].type != "order") {
        const UgdRecord& part = records[++i];
        in_.at(&part);
        if (!readLevelPart(part, *level, mask, maskRows)) return false;
    }
    in_.at(&r);
    if (maskRows != CollisionMask::HEIGHT)
        return in_.fail("level " + r.name + ": " + std::to_string(maskRows) + " mask rows");
    level->mask = CollisionMask{std::move(mask)};
    levels_[level->id] = level.get();
    data_.levels_.push_back(std::move(level));
    return true;
}

bool LevelReader::readLevelPart(const UgdRecord& r, LevelDefinition& level, std::vector<uint8_t>& mask, int& maskRows) {
    std::vector<int> v;
    int x = 0, y = 0, speed = 0, padIndex = 0;
    if (r.type == "pad") {
        if (!in_.only(r, {"left", "right", "y", "door", "wait", "stand", "number"})) return false;
        int p[7];
        const char* keys[7] = {"left", "right", "y", "door", "wait", "stand", "number"};
        for (int k = 0; k < 7; k++)
            if (!in_.number(r, keys[k], p[k])) return false;
        level.pads.push_back({p[0], p[1], p[2], p[3], p[4], p[5], p[6]});
    } else if (r.type == "routePassenger") {
        std::string kindName, route;
        if (!in_.only(r, {"kind", "route"}) || !in_.text(r, "kind", kindName) || !in_.text(r, "route", route)) return false;
        const RoutePassengerKind* kind = kinds_.routeKind(kindName);
        if (!kind) return in_.fail("no route passenger kind " + kindName);
        std::vector<Route::Stop> stops;
        std::vector<std::string> entries = RecordReader::split(route, ',');
        for (size_t e = 0; e < entries.size(); e++) {
            std::vector<std::string> padDelay = RecordReader::split(entries[e], '/');
            int padValue = 0, delay = 0;
            bool last = e + 1 == entries.size();
            if (padDelay.size() != (last ? 1u : 2u) || !RecordReader::parseInt(padDelay[0], padValue) ||
                (!last && !RecordReader::parseInt(padDelay[1], delay)) || !pad(level, padValue))
                return in_.fail("bad route " + route);
            if (!stops.empty()) stops.back().targetPad = padValue;
            if (!last) stops.push_back({padValue, delay, 0});
        }
        if (stops.empty()) return in_.fail("a route without a stop");
        level.passengers.push_back(std::make_unique<RoutePassengerPlacement>(*kind, Route{std::move(stops)}));
    } else if (r.type == "standingPassenger") {
        const StandingPassengerKind* kind = kinds_.standingKind("standing");
        if (!kind) return in_.fail("no passenger kind standing");
        if (!in_.only(r, {"x", "y"}) || !in_.number(r, "x", x) || !in_.number(r, "y", y)) return false;
        level.passengers.push_back(std::make_unique<StandingPassengerPlacement>(
            *kind, units::Fixed::fromRaw(x), units::Fixed::fromRaw(y)));
    } else if (r.type == "flyer") {
        int startDelay = 0;
        if (!in_.only(r, {"startDelay", "speed"}) || !in_.number(r, "startDelay", startDelay) ||
            !in_.number(r, "speed", speed))
            return false;
        level.enemies.push_back(std::make_unique<FlyerPlacement>(startDelay, units::Fixed::fromRaw(speed)));
    } else if (r.type == "walker") {
        if (!in_.only(r, {"pad", "x", "y", "speed"}) || !in_.number(r, "pad", padIndex) || !in_.number(r, "x", x) ||
            !in_.number(r, "y", y) || !in_.number(r, "speed", speed) || !pad(level, padIndex))
            return false;
        level.enemies.push_back(std::make_unique<WalkerPlacement>(padIndex, units::Fixed::fromRaw(x),
                                                                  units::Fixed::fromRaw(y), units::Fixed::fromRaw(speed)));
    } else if (r.type == "blower") {
        if (!in_.only(r, {"x", "y"}) || !in_.number(r, "x", x) || !in_.number(r, "y", y)) return false;
        level.enemies.push_back(std::make_unique<BlowerPlacement>(units::Fixed::fromRaw(x), units::Fixed::fromRaw(y)));
    } else if (r.type == "tree") {
        std::string dropText;
        if (!in_.only(r, {"x", "y", "drops"}) || !in_.number(r, "x", x) || !in_.number(r, "y", y) ||
            !in_.text(r, "drops", dropText))
            return false;
        std::vector<const BonusKind*> drops;
        if (dropText != "-") {
            for (const std::string& name : RecordReader::split(dropText, ',')) {
                const BonusKind* drop = kinds_.bonusKind(name);
                if (!drop) return in_.fail("no bonus kind " + name);
                drops.push_back(drop);
            }
        }
        level.enemies.push_back(std::make_unique<TreePlacement>(units::Fixed::fromRaw(x), units::Fixed::fromRaw(y), drops));
    } else if (r.type == "mask") {
        if (!r.fields.empty() || static_cast<int>(r.name.size()) != MASK_ROW_DIGITS || maskRows >= CollisionMask::HEIGHT)
            return in_.fail("a bad mask row");
        for (int d = 0; d < MASK_ROW_DIGITS; d += 2) {
            unsigned value = 0;
            auto [end, err] = std::from_chars(r.name.data() + d, r.name.data() + d + 2, value, 16);
            if (err != std::errc() || end != r.name.data() + d + 2) return in_.fail("a bad mask row");
            mask.push_back(static_cast<uint8_t>(value));
        }
        maskRows++;
    }
    return true;
}

bool LevelReader::readOrder(const UgdRecord& r) {
    for (int mode = 0; mode < 2; mode++) {
        const char* key = mode == 0 ? "oneplayer" : "team";
        if (!r.fields.count(key)) continue;
        if (!in_.only(r, {key})) return false;
        if (!data_.order_[mode].empty()) return in_.fail(std::string("a second order of ") + key);
        std::vector<int> ids;
        if (!in_.numbers(r, key, 0, ids)) return false;
        for (int id : ids) {
            auto it = levels_.find(id);
            if (it == levels_.end()) return in_.fail("no level " + std::to_string(id));
            data_.order_[mode].push_back(it->second);
        }
        return true;
    }
    return in_.fail("an order of no mode");
}

bool LevelReader::pad(const LevelDefinition& level, int index) {
    return (index >= 0 && index < static_cast<int>(level.pads.size())) || in_.fail("no pad " + std::to_string(index));
}

}  // namespace ugh::data
