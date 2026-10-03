// All the data of the game logic (Repository): what the loader made of the extractor's export.
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "core/Word.hpp"
#include "data/Animation.hpp"
#include "data/BonusKind.hpp"
#include "data/DataImage.hpp"
#include "data/DropList.hpp"
#include "data/EnemyKind.hpp"
#include "data/KeyBinding.hpp"
#include "data/LevelDefinition.hpp"
#include "data/PassengerKind.hpp"
#include "data/Route.hpp"
#include "data/Sprite.hpp"

namespace ugh::data {

/**
 * The levels, the kinds of passengers, enemies and bonus items, the animations, routes and drop lists, the key
 * table and the rules that depend on the difficulty. GameDataLoader makes it; nothing changes it afterwards.
 *
 * The objects keep the offset of their table in the original's data as `origin`: the identity the golden replays use
 * (passenger.N.kind=0x7720, route=0x3cf9 ...). The lookups by origin are for the replay projection.
 */
class GameData {
public:
    static constexpr int DIFFICULTIES = 3;
    static constexpr int PLAYERS = 2;

    /** Level `number` (from 0) of the one-player (1) or team (2) order; nullptr when there is none. */
    const LevelDefinition* level(int players, int number) const;
    int levelCount(int players) const;

    /** The impact that crashes a copter, by difficulty (0 .. 2). */
    core::Word crashLimit(int difficulty) const { return crashLimits_[difficulty]; }
    /** The highest score multiplier, by difficulty (0 .. 2). */
    core::Word multiplierLimit(int difficulty) const { return multiplierLimits_[difficulty]; }

    /** The rotor sprites of a player's copter: the first, and one past the last. */
    Sprite rotorFirst(int player) const { return rotorSprites_[player]; }
    Sprite rotorEnd(int player) const { return rotorSprites_[player + 1]; }

    const std::vector<KeyBinding>& keys() const { return keys_; }

    /** The bonus item a quick delivery drops (7a38). */
    const BonusKind& quickDeliveryBonus() const { return *quickDeliveryBonus_; }

    // lookups by origin (the replay projection); nullptr / false when there is none
    const PassengerKind* passengerKind(uint16_t origin) const;
    const EnemyKind* enemyKind(EnemyKind::Type type) const;
    const BonusKind* bonusKind(uint16_t origin) const;
    const Animation* animation(uint16_t origin) const;
    /** The position in a route a pointer of the original points to. */
    bool routeAt(uint16_t address, RouteCursor& cursor) const;
    /** The position in a drop list a pointer of the original points to. */
    bool dropsAt(uint16_t address, DropCursor& cursor) const;

private:
    friend class GameDataLoader;

    DataImage image_;   // the animations read it
    std::map<uint16_t, std::unique_ptr<Animation>> animations_;
    std::map<uint16_t, std::unique_ptr<PassengerKind>> passengerKinds_;
    std::map<uint16_t, std::unique_ptr<EnemyKind>> enemyKinds_;
    std::map<uint16_t, std::unique_ptr<BonusKind>> bonusKinds_;
    std::map<uint16_t, std::unique_ptr<Route>> routes_;
    std::map<uint16_t, std::unique_ptr<DropList>> drops_;
    std::map<uint16_t, std::unique_ptr<LevelDefinition>> levels_;   // by record
    std::array<std::vector<const LevelDefinition*>, PLAYERS> order_;   // one player, team
    std::vector<KeyBinding> keys_;
    std::array<core::Word, DIFFICULTIES> crashLimits_{};
    std::array<core::Word, DIFFICULTIES> multiplierLimits_{};
    std::array<Sprite, PLAYERS + 1> rotorSprites_{};   // player 0 from [0], player 1 from [1] (= the end of player 0)
    const BonusKind* quickDeliveryBonus_ = nullptr;
};

}  // namespace ugh::data
