#include "data/KindsReader.hpp"

namespace ugh::data {

namespace {

template <class Kind>
const Kind* find(const std::map<std::string, Kind*>& kinds, const std::string& name) {
    auto it = kinds.find(name);
    return it == kinds.end() ? nullptr : it->second;
}

}  // namespace

bool KindsReader::readAnimation(const UgdRecord& r) {
    std::vector<int> frames;
    if (!in_.only(r, {"frames"}) || !in_.numbers(r, "frames", 0, frames)) return false;
    if (r.name.empty() || frames.empty()) return in_.fail("an animation without a name or frames");
    if (animations_.count(r.name)) return in_.fail("animation " + r.name + " twice");
    data_.animations_.push_back(std::make_unique<Animation>(Animation{r.name, frames}));
    animations_[r.name] = data_.animations_.back().get();
    return true;
}

bool KindsReader::readPassengerKind(const UgdRecord& r) {
    std::string type;
    Box kindBox;
    if (r.name.empty() || passengerKindNames_.count(r.name)) return in_.fail("a passenger kind without a name or twice");
    if (!in_.text(r, "type", type) || !in_.box(r, kindBox)) return false;
    passengerKindNames_.insert(r.name);
    int look = 0;
    if (type == "route") {
        auto kind = std::make_unique<RoutePassengerKind>();
        if (!in_.only(r, {"type", "box", "standing", "waving", "walk", "comingOut", "goingIn", "animDelay", "fare",
                      "fareMin", "look", "waterKind"}) ||
            !animation(r, "comingOut", kind->comingOut) || !animation(r, "goingIn", kind->goingIn) ||
            !animated(r, *kind) || !in_.number(r, "look", look))
            return false;
        kind->look = look;
        named(*kind, r.name, kindBox);
        routeKinds_[r.name] = kind.get();
        data_.routePassengerKinds_.push_back(std::move(kind));
    } else if (type == "water") {
        auto kind = std::make_unique<SwimmerKind>();
        int swimTime = 0, rescuable = 0;
        if (!in_.only(r, {"type", "box", "standing", "waving", "walk", "animDelay", "fare", "fareMin", "swimTime",
                      "landKind", "rescuable"}) ||
            !in_.number(r, "swimTime", swimTime) || !in_.number(r, "rescuable", rescuable) || !animated(r, *kind))
            return false;
        kind->swimTime = swimTime;
        kind->rescuable = rescuable != 0;
        named(*kind, r.name, kindBox);
        swimmerKinds_[r.name] = kind.get();
        data_.swimmerKinds_.push_back(std::move(kind));
    } else if (type == "standing") {
        auto kind = std::make_unique<StandingPassengerKind>();
        if (!in_.only(r, {"type", "box", "look"}) || !in_.number(r, "look", look)) return false;
        kind->look = look;
        named(*kind, r.name, kindBox);
        standingKinds_[r.name] = kind.get();
        data_.standingPassengerKinds_.push_back(std::move(kind));
    } else {
        return in_.fail("unknown passenger type " + type);
    }
    return true;
}

/** What a passenger with a route shows and pays, on land and in the water. */
bool KindsReader::animated(const UgdRecord& r, AnimatedPassengerKind& kind) {
    int animDelay = 0, fare = 0, fareMin = 0;
    if (!animation(r, "standing", kind.standing) || !animation(r, "waving", kind.waving) ||
        !pair(r, "walk", kind.walking) || !in_.number(r, "animDelay", animDelay) || !in_.number(r, "fare", fare) ||
        !in_.number(r, "fareMin", fareMin))
        return false;
    kind.animDelay = animDelay;
    kind.fare = fare;
    kind.fareMin = fareMin;
    return true;
}

void KindsReader::named(PassengerKind& kind, const std::string& name, const Box& box) {
    kind.name = name;
    kind.box = box;
}

/** A route kind and its water kind name each other. */
bool KindsReader::linkPassengerKinds(const std::vector<UgdRecord>& records) {
    for (const UgdRecord& r : records) {
        if (r.type != "passengerKind") continue;
        in_.at(&r);
        auto route = routeKinds_.find(r.name);
        auto swimmer = swimmerKinds_.find(r.name);
        if (route == routeKinds_.end() && swimmer == swimmerKinds_.end()) continue;
        const char* key = route != routeKinds_.end() ? "waterKind" : "landKind";
        std::string other;
        if (!in_.text(r, key, other)) return false;
        if (!passengerKindNames_.count(other)) return in_.fail("no passenger kind " + other);
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

bool KindsReader::readBonusKind(const UgdRecord& r) {
    auto kind = std::make_unique<BonusKind>();
    kind->name = r.name;
    std::string effect;
    int amount = 0, lift = 0;
    std::vector<int> anchor;
    if (r.name.empty() || bonusKinds_.count(r.name)) return in_.fail("a bonus kind without a name or twice");
    if (!in_.only(r, {"effect", "amount", "lift", "sprite", "anchor"}) || !in_.text(r, "effect", effect) ||
        !in_.number(r, "amount", amount) || !in_.number(r, "lift", lift) || !in_.number(r, "sprite", kind->sprite) ||
        !in_.numbers(r, "anchor", 2, anchor))
        return false;
    if (effect == "energy") kind->effect = BonusEffect::Energy;
    else if (effect == "life") kind->effect = BonusEffect::Life;
    else if (effect == "multiplier") kind->effect = BonusEffect::Multiplier;
    else return in_.fail("unknown effect " + effect);
    kind->amount = amount;
    kind->lift = lift;
    kind->anchorX = anchor[0];
    kind->anchorY = anchor[1];
    bonusKinds_[r.name] = kind.get();
    data_.bonusKinds_.push_back(std::move(kind));
    return true;
}

bool KindsReader::readEnemyKind(const UgdRecord& r) {
    int score = 0;
    if (r.type == "flyerKind") {
        FlyerKind& k = data_.flyerKind_;
        std::vector<int> hit;
        if (!in_.only(r, {"box", "flight", "hitSprite", "score"}) || !in_.box(r, k.box) || !pair(r, "flight", k.flight) ||
            !in_.numbers(r, "hitSprite", 2, hit) || !in_.number(r, "score", score))
            return false;
        k.hitSpriteLeft = hit[0];
        k.hitSpriteRight = hit[1];
        k.score = score;
    } else if (r.type == "walkerKind") {
        WalkerKind& k = data_.walkerKind_;
        if (!in_.only(r, {"box", "walk", "watch", "charge", "recover", "stunned", "score"}) || !in_.box(r, k.box) ||
            !pair(r, "walk", k.walk) || !pair(r, "watch", k.watch) || !pair(r, "charge", k.charge) ||
            !pair(r, "recover", k.recover) || !pair(r, "stunned", k.stunned) || !in_.number(r, "score", score))
            return false;
        k.score = score;
    } else if (r.type == "blowerKind") {
        BlowerKind& k = data_.blowerKind_;
        if (!in_.only(r, {"box", "blowing", "stunnedSprite", "score"}) || !in_.box(r, k.box) ||
            !animation(r, "blowing", k.blowing) || !in_.number(r, "stunnedSprite", k.stunnedSprite) ||
            !in_.number(r, "score", score))
            return false;
        k.score = score;
    } else {
        if (!in_.only(r, {"swaying"}) || !animation(r, "swaying", data_.treeKind_.swaying)) return false;
    }
    return true;
}


bool KindsReader::animation(const UgdRecord& r, const char* key, const Animation*& out) {
    std::string name;
    if (!in_.text(r, key, name)) return false;
    auto it = animations_.find(name);
    if (it == animations_.end()) return in_.fail("no animation " + name);
    out = it->second;
    return true;
}

bool KindsReader::pair(const UgdRecord& r, const char* key, AnimationPair& out) {
    std::string value;
    if (!in_.text(r, key, value)) return false;
    std::vector<std::string> names = RecordReader::split(value, ',');
    if (names.size() != 2) return in_.fail(std::string(key) + " needs two animations");
    const Animation* sides[2] = {nullptr, nullptr};
    for (int s = 0; s < 2; s++) {
        auto it = animations_.find(names[s]);
        if (it == animations_.end()) return in_.fail("no animation " + names[s]);
        sides[s] = it->second;
    }
    out = {sides[0], sides[1]};
    return true;
}

const RoutePassengerKind* KindsReader::routeKind(const std::string& name) const { return find(routeKinds_, name); }

const StandingPassengerKind* KindsReader::standingKind(const std::string& name) const {
    return find(standingKinds_, name);
}

const BonusKind* KindsReader::bonusKind(const std::string& name) const { return find(bonusKinds_, name); }

}  // namespace ugh::data
