#include "data/ugd/RulesReader.hpp"

namespace ugh::data::ugd {

const RecordTable<RulesReader>::Entry RulesReader::RECORDS[] = {{"rules", &RulesReader::readRules},
                                                                {"sprites", &RulesReader::readSprites}};

bool RulesReader::reads(const std::string& type) { return RecordTable<RulesReader>::has(RECORDS, type); }

bool RulesReader::readAll(const std::vector<UgdRecord>& records) {
    for (const UgdRecord& r : records) {
        in_.at(&r);
        if (RecordTable<RulesReader>::has(RECORDS, r.type) && !RecordTable<RulesReader>::read(*this, RECORDS, r))
            return false;
    }
    in_.at(nullptr);
    return (rulesRead_ && spritesRead_) || in_.fail("rules or sprites missing");
}

bool RulesReader::readRules(const UgdRecord&) {
    std::vector<int> crash, multiplier;
    std::string bonus;
    if (rulesRead_) return in_.fail("a second rules record");
    if (!in_.only({"crashLimit", "multiplierLimit", "quickDeliveryBonus"}) ||
        !in_.numbers("crashLimit", DIFFICULTY_COUNT, crash) ||
        !in_.numbers("multiplierLimit", DIFFICULTY_COUNT, multiplier) ||
        !in_.text("quickDeliveryBonus", bonus))
        return false;
    const kinds::BonusKind* quickDeliveryBonus = kinds_.bonusKind(bonus);
    if (!quickDeliveryBonus) return in_.fail("no bonus kind " + bonus);
    Rules rules{{}, {}, quickDeliveryBonus};
    for (int d = 0; d < DIFFICULTY_COUNT; d++) {
        rules.crashLimits[d] = crash[d];
        rules.multiplierLimits[d] = multiplier[d];
    }
    data_.rules = rules;
    rulesRead_ = true;
    return true;
}

bool RulesReader::readSprites(const UgdRecord&) {
    SpriteIds& s = data_.sprites;
    if (spritesRead_) return in_.fail("a second sprites record");
    spritesRead_ = in_.only({"standingPassenger", "droppedPassenger", "bouncedPassenger", "shakenTree",
                             "destinationBubbles", "impatientBubble", "rotor0", "rotor1"}) &&
                   in_.number("standingPassenger", s.standingPassenger) &&
                   in_.number("droppedPassenger", s.droppedPassenger) &&
                   in_.number("bouncedPassenger", s.bouncedPassenger) && in_.number("shakenTree", s.shakenTree) &&
                   in_.range("destinationBubbles", s.firstDestinationBubble, s.lastDestinationBubble) &&
                   in_.number("impatientBubble", s.impatientBubble) &&
                   in_.range("rotor0", s.firstRotor[0], s.lastRotor[0]) &&
                   in_.range("rotor1", s.firstRotor[1], s.lastRotor[1]);
    return spritesRead_;
}

}  // namespace ugh::data::ugd
