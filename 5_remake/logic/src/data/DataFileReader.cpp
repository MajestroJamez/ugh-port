#include "data/DataFileReader.hpp"

#include <charconv>
#include <optional>
#include <fstream>
#include <iterator>
#include <sstream>

#include "data/BlowerPlacement.hpp"
#include "data/FlyerPlacement.hpp"
#include "data/RoutePassengerPlacement.hpp"
#include "data/StandingPassengerPlacement.hpp"
#include "data/TreePlacement.hpp"
#include "data/WalkerPlacement.hpp"

namespace ugh::data {

namespace {

constexpr std::string_view HEADER = "UGD 1";
constexpr int MASK_ROW_DIGITS = CollisionMask::WIDTH / 4;

}  // namespace

DataFileReader::DataFileReader() : data_(new GameData()) {}

std::unique_ptr<const GameData> DataFileReader::read(const std::string& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "cannot open " + path;
        return nullptr;
    }
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto data = parse(text, error);
    if (!data) error = path + ": " + error;
    return data;
}

std::unique_ptr<const GameData> DataFileReader::parse(std::string_view text, std::string& error) {
    std::vector<Record> records;
    std::istringstream in{std::string(text)};
    std::string line;
    int number = 0;
    bool header = false;
    while (std::getline(in, line)) {
        number++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!header) {
            if (line != HEADER) {
                error = "not a UGD 1 file";
                return nullptr;
            }
            header = true;
            continue;
        }
        if (line.empty() || line[0] == '#') continue;
        Record r;
        r.line = number;
        std::vector<std::string> words = split(line, ' ');
        r.type = words[0];
        bool firstWord = true;
        for (size_t w = 1; w < words.size(); w++) {
            if (words[w].empty()) continue;   // more spaces than one
            size_t eq = words[w].find('=');
            bool name = firstWord;
            firstWord = false;
            if (eq == std::string::npos) {
                if (!name) {
                    error = "line " + std::to_string(number) + ": a word without a value: " + words[w];
                    return nullptr;
                }
                r.name = words[w];
            } else {
                r.fields[words[w].substr(0, eq)] = words[w].substr(eq + 1);
            }
        }
        records.push_back(std::move(r));
    }
    if (!header) {
        error = "empty file";
        return nullptr;
    }
    DataFileReader reader;
    if (!reader.readAll(records)) {
        error = reader.error_;
        return nullptr;
    }
    return std::move(reader.data_);
}

bool DataFileReader::fail(const std::string& what) {
    error_ = current_ ? "line " + std::to_string(current_->line) + " (" + current_->type + "): " + what : what;
    return false;
}

/**
 * The records in the order their references need: animations, kinds, rules, levels, orders. The `key` records are the
 * keys of the PC keyboard: not for the logic (the replays' keyboard in 6_verification reads them).
 */
bool DataFileReader::readAll(const std::vector<Record>& records) {
    static const char* const KNOWN[] = {"rules", "sprites", "key", "animation", "passengerKind", "flyerKind",
                                        "walkerKind", "blowerKind", "treeKind", "bonusKind", "level", "pad",
                                        "routePassenger", "standingPassenger", "flyer", "walker", "blower", "tree",
                                        "mask", "order"};
    for (const Record& r : records) {
        current_ = &r;
        bool known = false;
        for (const char* type : KNOWN) known = known || r.type == type;
        if (!known) return fail("unknown record");
    }
    for (const Record& r : records) {
        current_ = &r;
        if (r.type == "animation" && !readAnimation(r)) return false;
    }
    for (const Record& r : records) {
        current_ = &r;
        if (r.type == "bonusKind" && !readBonusKind(r)) return false;
        if (r.type == "passengerKind" && !readPassengerKind(r)) return false;
        if ((r.type == "flyerKind" || r.type == "walkerKind" || r.type == "blowerKind" || r.type == "treeKind") &&
            !readEnemyKind(r))
            return false;
    }
    if (!linkPassengerKinds(records)) return false;
    bool rules = false, sprites = false;
    for (size_t i = 0; i < records.size(); i++) {
        const Record& r = records[i];
        current_ = &r;
        if (r.type == "rules") {
            if (!readRules(r)) return false;
            rules = true;
        } else if (r.type == "sprites") {
            if (!readSprites(r)) return false;
            sprites = true;
        } else if (r.type == "level") {
            if (!readLevel(records, i)) return false;
        } else if (r.type == "order") {
            if (!readOrder(r)) return false;
        } else if (r.type == "pad" || r.type == "routePassenger" || r.type == "standingPassenger" ||
                   r.type == "flyer" || r.type == "walker" || r.type == "blower" || r.type == "tree" || r.type == "mask") {
            return fail("outside a level");
        }
    }
    current_ = nullptr;
    if (!rules || !sprites) return fail("rules or sprites missing");
    if (data_->order_[0].empty() || data_->order_[1].empty()) return fail("a level order missing");
    return true;
}

bool DataFileReader::readAnimation(const Record& r) {
    std::vector<int> frames;
    if (!only(r, {"frames"}) || !numbers(r, "frames", 0, frames)) return false;
    if (r.name.empty() || frames.empty()) return fail("an animation without a name or frames");
    if (animations_.count(r.name)) return fail("animation " + r.name + " twice");
    data_->animations_.push_back(std::make_unique<Animation>(Animation{r.name, frames}));
    animations_[r.name] = data_->animations_.back().get();
    return true;
}

bool DataFileReader::readPassengerKind(const Record& r) {
    std::string type;
    Box kindBox;
    if (r.name.empty() || passengerKindNames_.count(r.name)) return fail("a passenger kind without a name or twice");
    if (!text(r, "type", type) || !box(r, kindBox)) return false;
    passengerKindNames_.insert(r.name);
    int look = 0;
    if (type == "route") {
        auto kind = std::make_unique<RoutePassengerKind>();
        if (!only(r, {"type", "box", "standing", "waving", "walk", "comingOut", "goingIn", "animDelay", "fare",
                      "fareMin", "look", "waterKind"}) ||
            !animation(r, "comingOut", kind->comingOut) || !animation(r, "goingIn", kind->goingIn) ||
            !animated(r, *kind) || !number(r, "look", look))
            return false;
        kind->look = look;
        named(*kind, r.name, kindBox);
        routeKinds_[r.name] = kind.get();
        data_->routePassengerKinds_.push_back(std::move(kind));
    } else if (type == "water") {
        auto kind = std::make_unique<SwimmerKind>();
        int swimTime = 0, rescuable = 0;
        if (!only(r, {"type", "box", "standing", "waving", "walk", "animDelay", "fare", "fareMin", "swimTime",
                      "landKind", "rescuable"}) ||
            !number(r, "swimTime", swimTime) || !number(r, "rescuable", rescuable) || !animated(r, *kind))
            return false;
        kind->swimTime = swimTime;
        kind->rescuable = rescuable != 0;
        named(*kind, r.name, kindBox);
        swimmerKinds_[r.name] = kind.get();
        data_->swimmerKinds_.push_back(std::move(kind));
    } else if (type == "standing") {
        auto kind = std::make_unique<StandingPassengerKind>();
        if (!only(r, {"type", "box", "look"}) || !number(r, "look", look)) return false;
        kind->look = look;
        named(*kind, r.name, kindBox);
        standingKinds_[r.name] = kind.get();
        data_->standingPassengerKinds_.push_back(std::move(kind));
    } else {
        return fail("unknown passenger type " + type);
    }
    return true;
}

/** What a passenger with a route shows and pays, on land and in the water. */
bool DataFileReader::animated(const Record& r, AnimatedPassengerKind& kind) {
    int animDelay = 0, fare = 0, fareMin = 0;
    if (!animation(r, "standing", kind.standing) || !animation(r, "waving", kind.waving) ||
        !pair(r, "walk", kind.walking) || !number(r, "animDelay", animDelay) || !number(r, "fare", fare) ||
        !number(r, "fareMin", fareMin))
        return false;
    kind.animDelay = animDelay;
    kind.fare = fare;
    kind.fareMin = fareMin;
    return true;
}

void DataFileReader::named(PassengerKind& kind, const std::string& name, const Box& box) {
    kind.name = name;
    kind.box = box;
}

/** A route kind and its water kind name each other. */
bool DataFileReader::linkPassengerKinds(const std::vector<Record>& records) {
    for (const Record& r : records) {
        if (r.type != "passengerKind") continue;
        current_ = &r;
        auto route = routeKinds_.find(r.name);
        auto swimmer = swimmerKinds_.find(r.name);
        if (route == routeKinds_.end() && swimmer == swimmerKinds_.end()) continue;
        const char* key = route != routeKinds_.end() ? "waterKind" : "landKind";
        std::string other;
        if (!text(r, key, other)) return false;
        if (!passengerKindNames_.count(other)) return fail("no passenger kind " + other);
        if (route != routeKinds_.end()) {
            auto it = swimmerKinds_.find(other);
            if (it == swimmerKinds_.end()) return fail(std::string(key) + " " + other + " is of the wrong type");
            route->second->swimmer = it->second;
        } else {
            auto it = routeKinds_.find(other);
            if (it == routeKinds_.end()) return fail(std::string(key) + " " + other + " is of the wrong type");
            swimmer->second->land = it->second;
        }
    }
    return true;
}

bool DataFileReader::readBonusKind(const Record& r) {
    auto kind = std::make_unique<BonusKind>();
    kind->name = r.name;
    std::string effect;
    int amount = 0, lift = 0;
    std::vector<int> anchor;
    if (r.name.empty() || bonusKinds_.count(r.name)) return fail("a bonus kind without a name or twice");
    if (!only(r, {"effect", "amount", "lift", "sprite", "anchor"}) || !text(r, "effect", effect) ||
        !number(r, "amount", amount) || !number(r, "lift", lift) || !number(r, "sprite", kind->sprite) ||
        !numbers(r, "anchor", 2, anchor))
        return false;
    if (effect == "energy") kind->effect = BonusEffect::Energy;
    else if (effect == "life") kind->effect = BonusEffect::Life;
    else if (effect == "multiplier") kind->effect = BonusEffect::Multiplier;
    else return fail("unknown effect " + effect);
    kind->amount = amount;
    kind->lift = lift;
    kind->anchorX = anchor[0];
    kind->anchorY = anchor[1];
    bonusKinds_[r.name] = kind.get();
    data_->bonusKinds_.push_back(std::move(kind));
    return true;
}

bool DataFileReader::readEnemyKind(const Record& r) {
    int score = 0;
    if (r.type == "flyerKind") {
        FlyerKind& k = data_->flyerKind_;
        std::vector<int> hit;
        if (!only(r, {"box", "flight", "hitSprite", "score"}) || !box(r, k.box) || !pair(r, "flight", k.flight) ||
            !numbers(r, "hitSprite", 2, hit) || !number(r, "score", score))
            return false;
        k.hitSpriteLeft = hit[0];
        k.hitSpriteRight = hit[1];
        k.score = score;
    } else if (r.type == "walkerKind") {
        WalkerKind& k = data_->walkerKind_;
        if (!only(r, {"box", "walk", "watch", "charge", "recover", "stunned", "score"}) || !box(r, k.box) ||
            !pair(r, "walk", k.walk) || !pair(r, "watch", k.watch) || !pair(r, "charge", k.charge) ||
            !pair(r, "recover", k.recover) || !pair(r, "stunned", k.stunned) || !number(r, "score", score))
            return false;
        k.score = score;
    } else if (r.type == "blowerKind") {
        BlowerKind& k = data_->blowerKind_;
        if (!only(r, {"box", "blowing", "stunnedSprite", "score"}) || !box(r, k.box) ||
            !animation(r, "blowing", k.blowing) || !number(r, "stunnedSprite", k.stunnedSprite) ||
            !number(r, "score", score))
            return false;
        k.score = score;
    } else {
        if (!only(r, {"swaying"}) || !animation(r, "swaying", data_->treeKind_.swaying)) return false;
    }
    return true;
}

bool DataFileReader::readRules(const Record& r) {
    std::vector<int> crash, multiplier;
    std::string bonus;
    if (!only(r, {"crashLimit", "multiplierLimit", "quickDeliveryBonus"}) || !numbers(r, "crashLimit", 3, crash) ||
        !numbers(r, "multiplierLimit", 3, multiplier) || !text(r, "quickDeliveryBonus", bonus))
        return false;
    auto it = bonusKinds_.find(bonus);
    if (it == bonusKinds_.end()) return fail("no bonus kind " + bonus);
    data_->rules_ = Rules{{crash[0], crash[1], crash[2]}, {multiplier[0], multiplier[1], multiplier[2]}, it->second};
    return true;
}

bool DataFileReader::readSprites(const Record& r) {
    SpriteIds& s = data_->sprites_;
    return only(r, {"standingPassenger", "droppedPassenger", "bouncedPassenger", "shakenTree", "destinationBubbles",
                    "impatientBubble", "rotor0", "rotor1"}) &&
           number(r, "standingPassenger", s.standingPassenger) && number(r, "droppedPassenger", s.droppedPassenger) &&
           number(r, "bouncedPassenger", s.bouncedPassenger) && number(r, "shakenTree", s.shakenTree) &&
           range(r, "destinationBubbles", s.firstDestinationBubble, s.lastDestinationBubble) &&
           number(r, "impatientBubble", s.impatientBubble) && range(r, "rotor0", s.firstRotor[0], s.lastRotor[0]) &&
           range(r, "rotor1", s.firstRotor[1], s.lastRotor[1]);
}

/** A level record and the records after it up to the next level or order. */
bool DataFileReader::readLevel(const std::vector<Record>& records, size_t& i) {
    const Record& r = records[i];
    auto level = std::make_unique<LevelDefinition>();
    std::string wind;
    std::vector<int> start0, start1;
    int water = 0, waterSpeed = 0;
    if (!parseInt(r.name, level->id)) return fail("a level without a number");
    if (levels_.count(level->id)) return fail("level " + r.name + " twice");
    if (!only(r, {"toDeliver", "wind", "start0", "start1", "water", "waterSpeed"}) ||
        !number(r, "toDeliver", level->toDeliver) || !text(r, "wind", wind) || !numbers(r, "start0", 2, start0) ||
        !numbers(r, "start1", 2, start1) || !number(r, "water", water) || !number(r, "waterSpeed", waterSpeed))
        return false;
    if (wind == "none") level->wind = Wind::None;
    else if (wind == "left") level->wind = Wind::Left;
    else if (wind == "right") level->wind = Wind::Right;
    else return fail("unknown wind " + wind);
    level->startX = {units::Fixed::fromRaw(start0[0]), units::Fixed::fromRaw(start1[0])};
    level->startY = {units::Fixed::fromRaw(start0[1]), units::Fixed::fromRaw(start1[1])};
    level->water = units::Fixed::fromRaw(water);
    level->waterSpeed = units::Fixed::fromRaw(waterSpeed);
    std::vector<uint8_t> mask;
    int maskRows = 0;
    while (i + 1 < records.size() && records[i + 1].type != "level" && records[i + 1].type != "order") {
        const Record& part = records[++i];
        current_ = &part;
        if (!readLevelPart(part, *level, mask, maskRows)) return false;
    }
    current_ = &r;
    if (maskRows != CollisionMask::HEIGHT) return fail("level " + r.name + ": " + std::to_string(maskRows) + " mask rows");
    level->mask = CollisionMask{std::move(mask)};
    levels_[level->id] = level.get();
    data_->levels_.push_back(std::move(level));
    return true;
}

bool DataFileReader::readLevelPart(const Record& r, LevelDefinition& level, std::vector<uint8_t>& mask, int& maskRows) {
    std::vector<int> v;
    int x = 0, y = 0, speed = 0, padIndex = 0;
    if (r.type == "pad") {
        if (!only(r, {"left", "right", "y", "door", "wait", "stand", "number"})) return false;
        int p[7];
        const char* keys[7] = {"left", "right", "y", "door", "wait", "stand", "number"};
        for (int k = 0; k < 7; k++)
            if (!number(r, keys[k], p[k])) return false;
        level.pads.push_back({p[0], p[1], p[2], p[3], p[4], p[5], p[6]});
    } else if (r.type == "routePassenger") {
        std::string kindName, route;
        if (!only(r, {"kind", "route"}) || !text(r, "kind", kindName) || !text(r, "route", route)) return false;
        auto kind = routeKinds_.find(kindName);
        if (kind == routeKinds_.end())
            return fail("no route passenger kind " + kindName);
        std::vector<Route::Stop> stops;
        std::vector<std::string> entries = split(route, ',');
        for (size_t e = 0; e < entries.size(); e++) {
            std::vector<std::string> padDelay = split(entries[e], '/');
            int padValue = 0, delay = 0;
            bool last = e + 1 == entries.size();
            if (padDelay.size() != (last ? 1u : 2u) || !parseInt(padDelay[0], padValue) ||
                (!last && !parseInt(padDelay[1], delay)) || !pad(level, padValue))
                return fail("bad route " + route);
            if (!stops.empty()) stops.back().targetPad = padValue;
            if (!last) stops.push_back({padValue, delay, 0});
        }
        if (stops.empty()) return fail("a route without a stop");
        level.passengers.push_back(std::make_unique<RoutePassengerPlacement>(*kind->second, Route{std::move(stops)}));
    } else if (r.type == "standingPassenger") {
        auto kind = standingKinds_.find("standing");
        if (kind == standingKinds_.end()) return fail("no passenger kind standing");
        if (!only(r, {"x", "y"}) || !number(r, "x", x) || !number(r, "y", y)) return false;
        level.passengers.push_back(std::make_unique<StandingPassengerPlacement>(
            *kind->second, units::Fixed::fromRaw(x), units::Fixed::fromRaw(y)));
    } else if (r.type == "flyer") {
        int startDelay = 0;
        if (!only(r, {"startDelay", "speed"}) || !number(r, "startDelay", startDelay) || !number(r, "speed", speed))
            return false;
        level.enemies.push_back(std::make_unique<FlyerPlacement>(startDelay, units::Fixed::fromRaw(speed)));
    } else if (r.type == "walker") {
        if (!only(r, {"pad", "x", "y", "speed"}) || !number(r, "pad", padIndex) || !number(r, "x", x) ||
            !number(r, "y", y) || !number(r, "speed", speed) || !pad(level, padIndex))
            return false;
        level.enemies.push_back(std::make_unique<WalkerPlacement>(padIndex, units::Fixed::fromRaw(x),
                                                                  units::Fixed::fromRaw(y), units::Fixed::fromRaw(speed)));
    } else if (r.type == "blower") {
        if (!only(r, {"x", "y"}) || !number(r, "x", x) || !number(r, "y", y)) return false;
        level.enemies.push_back(std::make_unique<BlowerPlacement>(units::Fixed::fromRaw(x), units::Fixed::fromRaw(y)));
    } else if (r.type == "tree") {
        std::string dropText;
        if (!only(r, {"x", "y", "drops"}) || !number(r, "x", x) || !number(r, "y", y) || !text(r, "drops", dropText))
            return false;
        std::vector<const BonusKind*> drops;
        if (dropText != "-") {
            for (const std::string& name : split(dropText, ',')) {
                auto it = bonusKinds_.find(name);
                if (it == bonusKinds_.end()) return fail("no bonus kind " + name);
                drops.push_back(it->second);
            }
        }
        level.enemies.push_back(std::make_unique<TreePlacement>(units::Fixed::fromRaw(x), units::Fixed::fromRaw(y), drops));
    } else if (r.type == "mask") {
        if (!r.fields.empty() || static_cast<int>(r.name.size()) != MASK_ROW_DIGITS || maskRows >= CollisionMask::HEIGHT)
            return fail("a bad mask row");
        for (int d = 0; d < MASK_ROW_DIGITS; d += 2) {
            unsigned value = 0;
            auto [end, err] = std::from_chars(r.name.data() + d, r.name.data() + d + 2, value, 16);
            if (err != std::errc() || end != r.name.data() + d + 2) return fail("a bad mask row");
            mask.push_back(static_cast<uint8_t>(value));
        }
        maskRows++;
    }
    return true;
}

bool DataFileReader::readOrder(const Record& r) {
    for (int mode = 0; mode < 2; mode++) {
        const char* key = mode == 0 ? "oneplayer" : "team";
        if (!r.fields.count(key)) continue;
        if (!only(r, {key})) return false;
        if (!data_->order_[mode].empty()) return fail(std::string("a second order of ") + key);
        std::vector<int> ids;
        if (!numbers(r, key, 0, ids)) return false;
        for (int id : ids) {
            auto it = levels_.find(id);
            if (it == levels_.end()) return fail("no level " + std::to_string(id));
            data_->order_[mode].push_back(it->second);
        }
        return true;
    }
    return fail("an order of no mode");
}

bool DataFileReader::only(const Record& r, std::initializer_list<const char*> keys) {
    for (const auto& [key, value] : r.fields) {
        bool known = false;
        for (const char* k : keys) known = known || key == k;
        if (!known) return fail("unknown key " + key);
    }
    return true;
}

bool DataFileReader::text(const Record& r, const char* key, std::string& out) {
    auto it = r.fields.find(key);
    if (it == r.fields.end()) return fail(std::string("no ") + key);
    out = it->second;
    return true;
}

bool DataFileReader::number(const Record& r, const char* key, int& out) {
    std::string value;
    if (!text(r, key, value)) return false;
    return parseInt(value, out) || fail(std::string("bad number ") + key + "=" + value);
}

bool DataFileReader::numbers(const Record& r, const char* key, size_t count, std::vector<int>& out) {
    std::string value;
    if (!text(r, key, value)) return false;
    out.clear();
    for (const std::string& part : split(value, ',')) {
        int v = 0;
        if (!parseInt(part, v)) return fail(std::string("bad numbers ") + key + "=" + value);
        out.push_back(v);
    }
    if (count != 0 && out.size() != count) return fail(std::string(key) + " needs " + std::to_string(count) + " numbers");
    return true;
}

bool DataFileReader::range(const Record& r, const char* key, int& first, int& last) {
    std::string value;
    if (!text(r, key, value)) return false;
    size_t dots = value.find("..");
    if (dots == std::string::npos || !parseInt(value.substr(0, dots), first) || !parseInt(value.substr(dots + 2), last) ||
        last < first)
        return fail(std::string("bad range ") + key + "=" + value);
    return true;
}

bool DataFileReader::box(const Record& r, Box& out) {
    std::vector<int> v;
    if (!numbers(r, "box", 4, v)) return false;
    out = {v[0], v[1], v[2], v[3]};
    return true;
}

bool DataFileReader::animation(const Record& r, const char* key, const Animation*& out) {
    std::string name;
    if (!text(r, key, name)) return false;
    auto it = animations_.find(name);
    if (it == animations_.end()) return fail("no animation " + name);
    out = it->second;
    return true;
}

bool DataFileReader::pair(const Record& r, const char* key, AnimationPair& out) {
    std::string value;
    if (!text(r, key, value)) return false;
    std::vector<std::string> names = split(value, ',');
    if (names.size() != 2) return fail(std::string(key) + " needs two animations");
    const Animation* sides[2] = {nullptr, nullptr};
    for (int s = 0; s < 2; s++) {
        auto it = animations_.find(names[s]);
        if (it == animations_.end()) return fail("no animation " + names[s]);
        sides[s] = it->second;
    }
    out = {sides[0], sides[1]};
    return true;
}

bool DataFileReader::pad(const LevelDefinition& level, int index) {
    return (index >= 0 && index < static_cast<int>(level.pads.size())) || fail("no pad " + std::to_string(index));
}

bool DataFileReader::parseInt(const std::string& text, int& out) {
    auto [end, err] = std::from_chars(text.data(), text.data() + text.size(), out);
    return err == std::errc() && end == text.data() + text.size() && !text.empty();
}

std::vector<std::string> DataFileReader::split(const std::string& text, char separator) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t at = text.find(separator, start);
        parts.push_back(text.substr(start, at - start));
        if (at == std::string::npos) return parts;
        start = at + 1;
    }
}

}  // namespace ugh::data
