#include "data/GameData.hpp"

#include <iterator>

namespace ugh::data {

const LevelDefinition* GameData::level(int players, int number) const {
    if (players < 1 || players > PLAYERS) return nullptr;
    const auto& order = order_[players - 1];
    return number >= 0 && number < static_cast<int>(order.size()) ? order[number] : nullptr;
}

int GameData::levelCount(int players) const {
    return players >= 1 && players <= PLAYERS ? static_cast<int>(order_[players - 1].size()) : 0;
}

const PassengerKind* GameData::passengerKind(uint16_t origin) const {
    auto it = passengerKinds_.find(origin);
    return it == passengerKinds_.end() ? nullptr : it->second.get();
}

const EnemyKind* GameData::enemyKind(EnemyKind::Type type) const {
    for (const auto& [origin, kind] : enemyKinds_)
        if (kind->type == type) return kind.get();
    return nullptr;
}

const BonusKind* GameData::bonusKind(uint16_t origin) const {
    auto it = bonusKinds_.find(origin);
    return it == bonusKinds_.end() ? nullptr : it->second.get();
}

const Animation* GameData::animation(uint16_t origin) const {
    auto it = animations_.find(origin);
    return it == animations_.end() ? nullptr : it->second.get();
}

bool GameData::routeAt(uint16_t address, RouteCursor& cursor) const {
    // the route that starts last at or before the address (a route starting inside another one is its tail)
    auto it = routes_.upper_bound(address);
    if (it == routes_.begin()) return false;
    const Route& route = *std::prev(it)->second;
    int offset = address - route.origin;
    if (offset % 4 != 0 || 2 * (offset / 4) + 2 >= static_cast<int>(route.words.size())) return false;
    cursor = {&route, offset / 4};
    return true;
}

bool GameData::dropsAt(uint16_t address, DropCursor& cursor) const {
    for (const auto& [origin, list] : drops_) {
        int offset = address - origin;
        if (offset < 0 || offset % 2 != 0 || offset / 2 > static_cast<int>(list->items.size())) continue;
        cursor = {list.get(), offset / 2};
        return true;
    }
    return false;
}

}  // namespace ugh::data
