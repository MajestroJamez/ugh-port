#include "data/GameData.hpp"

namespace ugh::data {

const levels::LevelDefinition* GameData::level(int players, int number) const {
    const auto& inOrder = order(players);
    return number >= 0 && number < static_cast<int>(inOrder.size()) ? inOrder[number] : nullptr;
}

int GameData::levelCount(int players) const { return static_cast<int>(order(players).size()); }

}  // namespace ugh::data
