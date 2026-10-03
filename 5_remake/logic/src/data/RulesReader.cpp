#include "data/RulesReader.hpp"

namespace ugh::data {

bool RulesReader::readRules(const UgdRecord& r) {
    std::vector<int> crash, multiplier;
    std::string bonus;
    if (!in_.only(r, {"crashLimit", "multiplierLimit", "quickDeliveryBonus"}) ||
        !in_.numbers(r, "crashLimit", 3, crash) || !in_.numbers(r, "multiplierLimit", 3, multiplier) ||
        !in_.text(r, "quickDeliveryBonus", bonus))
        return false;
    const BonusKind* quickDeliveryBonus = kinds_.bonusKind(bonus);
    if (!quickDeliveryBonus) return in_.fail("no bonus kind " + bonus);
    data_.rules_ =
        Rules{{crash[0], crash[1], crash[2]}, {multiplier[0], multiplier[1], multiplier[2]}, quickDeliveryBonus};
    return true;
}

bool RulesReader::readSprites(const UgdRecord& r) {
    SpriteIds& s = data_.sprites_;
    return in_.only(r, {"standingPassenger", "droppedPassenger", "bouncedPassenger", "shakenTree",
                        "destinationBubbles", "impatientBubble", "rotor0", "rotor1"}) &&
           in_.number(r, "standingPassenger", s.standingPassenger) &&
           in_.number(r, "droppedPassenger", s.droppedPassenger) &&
           in_.number(r, "bouncedPassenger", s.bouncedPassenger) && in_.number(r, "shakenTree", s.shakenTree) &&
           in_.range(r, "destinationBubbles", s.firstDestinationBubble, s.lastDestinationBubble) &&
           in_.number(r, "impatientBubble", s.impatientBubble) &&
           in_.range(r, "rotor0", s.firstRotor[0], s.lastRotor[0]) &&
           in_.range(r, "rotor1", s.firstRotor[1], s.lastRotor[1]);
}

}  // namespace ugh::data
