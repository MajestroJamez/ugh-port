#include "data/GameData.hpp"

namespace ugh::data {

const levels::LevelDefinition* GameData::level(int players, int number) const {
    const auto& order = contents_.order[players == 2 ? 1 : 0];
    return number >= 0 && number < static_cast<int>(order.size()) ? order[number] : nullptr;
}

int GameData::levelCount(int players) const { return static_cast<int>(contents_.order[players == 2 ? 1 : 0].size()); }

}  // namespace ugh::data
