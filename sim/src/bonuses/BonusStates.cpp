#include "bonuses/BonusStates.hpp"

#include "bonuses/Falling.hpp"
#include "bonuses/Lying.hpp"

namespace ugh::bonuses {

const std::vector<const BonusState*>& BonusStates::all() {
    static const std::vector<const BonusState*> states = {&Falling::instance, &Lying::instance};
    return states;
}

}  // namespace ugh::bonuses
