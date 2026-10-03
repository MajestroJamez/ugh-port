#include "TestFramework.hpp"
#include "data/GameData.hpp"

using namespace ugh;

TEST(levelsLoaded) {
    const data::GameData& game = test::gameData();
    CHECK(game.levelCount(1) == 69 && game.levelCount(2) == 81);
    CHECK(game.level(1, 69) == nullptr);
    const data::LevelDefinition& level = *game.level(1, 0);
    CHECK(level.pads.size() == 3 && level.passengers.size() == 3 && level.enemies.size() == 1);
    CHECK(level.startX[0] == core::Fixed(4608) && level.startY[0] == core::Fixed(2080));
    CHECK(level.enemies[0].kind->type == data::EnemyKind::Type::Tree && level.enemies[0].drops != nullptr);
}

TEST(kindsLinked) {
    using Type = data::PassengerKind::Type;
    const data::GameData& game = test::gameData();
    const data::PassengerKind& walking = *game.passengerKind(0x7720);
    CHECK(walking.type == Type::Walking && walking.other->type == Type::Swimming);
    CHECK(walking.other->other == &walking);
    CHECK(!game.passengerKind(0x77fe)->rescuable && walking.other->rescuable);
    CHECK(game.quickDeliveryBonus().effect == data::BonusKind::Effect::Multiplier);
    CHECK(game.rotorEnd(0) == game.rotorFirst(1));
}
