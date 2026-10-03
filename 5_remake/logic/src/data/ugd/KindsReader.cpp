#include "data/ugd/KindsReader.hpp"

#include <memory>

namespace ugh::data::ugd {

namespace {

constexpr const char* ANIMATION = "animation";
// the kinds of the enemies: each once
constexpr const char* FLYER_KIND = "flyerKind";
constexpr const char* WALKER_KIND = "walkerKind";
constexpr const char* BLOWER_KIND = "blowerKind";
constexpr const char* TREE_KIND = "treeKind";
constexpr const char* ENEMY_KINDS[] = {FLYER_KIND, WALKER_KIND, BLOWER_KIND, TREE_KIND};

}  // namespace

const RecordTable<KindsReader>::Entry KindsReader::KINDS[] = {
    {PassengerKindsReader::RECORD, &KindsReader::readPassengerKind}, {"bonusKind", &KindsReader::readBonusKind},
    {FLYER_KIND, &KindsReader::readFlyerKind},   {WALKER_KIND, &KindsReader::readWalkerKind},
    {BLOWER_KIND, &KindsReader::readBlowerKind}, {TREE_KIND, &KindsReader::readTreeKind}};

bool KindsReader::reads(const std::string& type) { return type == ANIMATION || RecordTable<KindsReader>::has(KINDS, type); }

bool KindsReader::readAll(const std::vector<UgdRecord>& records) {
    for (const UgdRecord& r : records) {
        in_.at(&r);
        if (r.type == ANIMATION && !animations_.readAnimation(r)) return false;
    }
    for (const UgdRecord& r : records) {
        in_.at(&r);
        if (RecordTable<KindsReader>::has(KINDS, r.type) && !RecordTable<KindsReader>::read(*this, KINDS, r)) return false;
    }
    in_.at(nullptr);
    for (const char* type : ENEMY_KINDS)
        if (!enemyKinds_.count(type)) return in_.fail(std::string("no ") + type);
    return passengers_.link(records);
}

bool KindsReader::readBonusKind(const UgdRecord& r) {
    auto kind = std::make_unique<kinds::BonusKind>();
    kind->name = r.name;
    std::string effect;
    std::vector<int> anchor;
    int lift = 0;
    if (r.name.empty() || bonusKinds_.count(r.name)) return in_.fail("a bonus kind without a name or twice");
    if (!in_.only({"effect", "amount", "lift", "sprite", "anchor"}) || !in_.text("effect", effect) ||
        !in_.number("amount", kind->amount) || !in_.number("lift", lift) || !in_.number("sprite", kind->sprite) ||
        !in_.numbers("anchor", 2, anchor))
        return false;
    if (effect == "energy") kind->effect = kinds::BonusEffect::Energy;
    else if (effect == "life") kind->effect = kinds::BonusEffect::Life;
    else if (effect == "multiplier") kind->effect = kinds::BonusEffect::Multiplier;
    else return in_.fail("unknown effect " + effect);
    kind->lift = units::Fixed::fromRaw(lift);
    kind->anchorX = anchor[0];
    kind->anchorY = anchor[1];
    bonusKinds_[r.name] = kind.get();
    data_.bonusKinds.push_back(std::move(kind));
    return true;
}

bool KindsReader::readFlyerKind(const UgdRecord&) {
    if (!once(FLYER_KIND)) return false;
    kinds::FlyerKind& k = data_.flyerKind;
    std::vector<int> hit;
    if (!in_.only({"box", "flight", "hitSprite", "score"}) || !in_.box(k.box) || !animations_.pair("flight", k.flight) ||
        !in_.numbers("hitSprite", 2, hit) || !in_.number("score", k.score))
        return false;
    k.hitSpriteLeft = hit[0];
    k.hitSpriteRight = hit[1];
    return true;
}

bool KindsReader::readWalkerKind(const UgdRecord&) {
    kinds::WalkerKind& k = data_.walkerKind;
    return once(WALKER_KIND) && in_.only({"box", "walk", "watch", "charge", "recover", "stunned", "score"}) &&
           in_.box(k.box) && animations_.pair("walk", k.walk) && animations_.pair("watch", k.watch) &&
           animations_.pair("charge", k.charge) && animations_.pair("recover", k.recover) &&
           animations_.pair("stunned", k.stunned) && in_.number("score", k.score);
}

bool KindsReader::readBlowerKind(const UgdRecord&) {
    kinds::BlowerKind& k = data_.blowerKind;
    return once(BLOWER_KIND) && in_.only({"box", "blowing", "stunnedSprite", "score"}) && in_.box(k.box) &&
           animations_.animation("blowing", k.blowing) && in_.number("stunnedSprite", k.stunnedSprite) &&
           in_.number("score", k.score);
}

bool KindsReader::readTreeKind(const UgdRecord&) {
    return once(TREE_KIND) && in_.only({"swaying"}) && animations_.animation("swaying", data_.treeKind.swaying);
}

bool KindsReader::once(const char* type) {
    return enemyKinds_.insert(type).second || in_.fail(std::string("a second ") + type);
}

const kinds::BonusKind* KindsReader::bonusKind(const std::string& name) const {
    auto it = bonusKinds_.find(name);
    return it == bonusKinds_.end() ? nullptr : it->second;
}

}  // namespace ugh::data::ugd
