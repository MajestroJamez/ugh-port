// All the data of the game.
#pragma once

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "data/Rules.hpp"
#include "data/SpriteIds.hpp"
#include "data/kinds/Animation.hpp"
#include "data/kinds/BlowerKind.hpp"
#include "data/kinds/BonusKind.hpp"
#include "data/kinds/FlyerKind.hpp"
#include "data/kinds/RoutePassengerKind.hpp"
#include "data/kinds/StandingPassengerKind.hpp"
#include "data/kinds/SwimmerKind.hpp"
#include "data/kinds/TreeKind.hpp"
#include "data/kinds/WalkerKind.hpp"
#include "data/levels/LevelDefinition.hpp"

namespace ugh::data {

/**
 * The data of the game (Repository), read-only: the levels in the order of both modes, the kinds of passengers,
 * enemies and bonus items, the animations and the rules. DataFileReader fills its `Contents` and makes it.
 */
class GameData {
public:
    /**
     * What the data holds, as the reader puts it together. The kinds and levels are owned here; whatever points to
     * them (an order, a placement, a rule) stays valid when the contents move into the GameData.
     */
    struct Contents {
        std::vector<std::unique_ptr<kinds::Animation>> animations;
        std::vector<std::unique_ptr<kinds::RoutePassengerKind>> routePassengerKinds;
        std::vector<std::unique_ptr<kinds::SwimmerKind>> swimmerKinds;
        std::vector<std::unique_ptr<kinds::StandingPassengerKind>> standingPassengerKinds;
        std::vector<std::unique_ptr<kinds::BonusKind>> bonusKinds;
        kinds::FlyerKind flyerKind;
        kinds::WalkerKind walkerKind;
        kinds::BlowerKind blowerKind;
        kinds::TreeKind treeKind;
        // the levels in the order of the file (the orders point into it)
        std::vector<std::unique_ptr<levels::LevelDefinition>> levelDefinitions;
        std::array<std::vector<const levels::LevelDefinition*>, 2> order;   // one player, team
        std::optional<Rules> rules;
        SpriteIds sprites;
    };

    /** The data of `contents`, which has its rules and both orders (DataFileReader checks it). */
    explicit GameData(Contents contents) : contents_(std::move(contents)) {}
    GameData(const GameData&) = delete;
    GameData& operator=(const GameData&) = delete;

    /** Level `number` (from 0) of the one-player (players 1) or the team order (players 2); nullptr past the last. */
    const levels::LevelDefinition* level(int players, int number) const;
    int levelCount(int players) const;

    const Rules& rules() const { return *contents_.rules; }
    const SpriteIds& sprites() const { return contents_.sprites; }

    const kinds::FlyerKind& flyerKind() const { return contents_.flyerKind; }
    const kinds::WalkerKind& walkerKind() const { return contents_.walkerKind; }
    const kinds::BlowerKind& blowerKind() const { return contents_.blowerKind; }
    const kinds::TreeKind& treeKind() const { return contents_.treeKind; }

private:
    Contents contents_;

    /** The order of the levels for `players`: the team order for 2, else the one-player order. */
    const std::vector<const levels::LevelDefinition*>& order(int players) const {
        return contents_.order[players == 2 ? 1 : 0];
    }
};

}  // namespace ugh::data
