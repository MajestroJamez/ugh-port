#include "data/ugd/RulesReader.hpp"

namespace ugh::data::ugd {

namespace {

constexpr const char* RULES = "rules";
constexpr const char* SPRITES = "sprites";

}  // namespace

bool RulesReader::reads(const std::string& type) { return type == RULES || type == SPRITES; }

bool RulesReader::readAll(const std::vector<UgdRecord>& records) {
    bool rulesRead = false, spritesRead = false;
    for (const UgdRecord& r : records) {
        in_.at(&r);
        if (r.type == RULES) {
            if (!readRules()) return false;
            rulesRead = true;
        } else if (r.type == SPRITES) {
            if (!readSprites()) return false;
            spritesRead = true;
        }
    }
    in_.at(nullptr);
    return (rulesRead && spritesRead) || in_.fail("rules or sprites missing");
}

bool RulesReader::readRules() {
    std::vector<int> crash, multiplier;
    std::string bonus;
    if (!in_.only({"crashLimit", "multiplierLimit", "quickDeliveryBonus"}) ||
        !in_.numbers("crashLimit", 3, crash) || !in_.numbers("multiplierLimit", 3, multiplier) ||
        !in_.text("quickDeliveryBonus", bonus))
        return false;
    const kinds::BonusKind* quickDeliveryBonus = kinds_.bonusKind(bonus);
    if (!quickDeliveryBonus) return in_.fail("no bonus kind " + bonus);
    data_.rules =
        Rules{{crash[0], crash[1], crash[2]}, {multiplier[0], multiplier[1], multiplier[2]}, quickDeliveryBonus};
    return true;
}

bool RulesReader::readSprites() {
    SpriteIds& s = data_.sprites;
    return in_.only({"standingPassenger", "droppedPassenger", "bouncedPassenger", "shakenTree", "destinationBubbles",
                     "impatientBubble", "rotor0", "rotor1"}) &&
           in_.number("standingPassenger", s.standingPassenger) &&
           in_.number("droppedPassenger", s.droppedPassenger) &&
           in_.number("bouncedPassenger", s.bouncedPassenger) && in_.number("shakenTree", s.shakenTree) &&
           in_.range("destinationBubbles", s.firstDestinationBubble, s.lastDestinationBubble) &&
           in_.number("impatientBubble", s.impatientBubble) &&
           in_.range("rotor0", s.firstRotor[0], s.lastRotor[0]) &&
           in_.range("rotor1", s.firstRotor[1], s.lastRotor[1]);
}

}  // namespace ugh::data::ugd
