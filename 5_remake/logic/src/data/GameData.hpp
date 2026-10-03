// All the data of the game.
#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "data/Animation.hpp"
#include "data/BlowerKind.hpp"
#include "data/BonusKind.hpp"
#include "data/FlyerKind.hpp"
#include "data/LevelDefinition.hpp"
#include "data/RoutePassengerKind.hpp"
#include "data/Rules.hpp"
#include "data/SpriteIds.hpp"
#include "data/StandingPassengerKind.hpp"
#include "data/SwimmerKind.hpp"
#include "data/TreeKind.hpp"
#include "data/WalkerKind.hpp"

namespace ugh::data {

class DataFileReader;
class KindsReader;
class LevelReader;
class RulesReader;

/**
 * The data of the game (Repository), read-only: the levels in the order of both modes, the kinds of passengers,
 * enemies and bonus items, the animations and the rules. DataFileReader makes it.
 */
class GameData {
public:
    GameData(const GameData&) = delete;
    GameData& operator=(const GameData&) = delete;

    /** Level `number` (from 0) of the one-player (players 1) or the team order (players 2); nullptr past the last. */
    const LevelDefinition* level(int players, int number) const;
    int levelCount(int players) const;

    const Rules& rules() const { return *rules_; }
    const SpriteIds& sprites() const { return sprites_; }

    const FlyerKind& flyerKind() const { return flyerKind_; }
    const WalkerKind& walkerKind() const { return walkerKind_; }
    const BlowerKind& blowerKind() const { return blowerKind_; }
    const TreeKind& treeKind() const { return treeKind_; }

private:
    friend class DataFileReader;
    friend class KindsReader;
    friend class LevelReader;
    friend class RulesReader;
    GameData() = default;

    std::vector<std::unique_ptr<Animation>> animations_;
    std::vector<std::unique_ptr<RoutePassengerKind>> routePassengerKinds_;
    std::vector<std::unique_ptr<SwimmerKind>> swimmerKinds_;
    std::vector<std::unique_ptr<StandingPassengerKind>> standingPassengerKinds_;
    std::vector<std::unique_ptr<BonusKind>> bonusKinds_;
    FlyerKind flyerKind_;
    WalkerKind walkerKind_;
    BlowerKind blowerKind_;
    TreeKind treeKind_;
    std::vector<std::unique_ptr<LevelDefinition>> levels_;   // in the order of the file (the orders point into it)
    std::array<std::vector<const LevelDefinition*>, 2> order_;   // one player, team
    std::optional<Rules> rules_;
    SpriteIds sprites_;
};

}  // namespace ugh::data
