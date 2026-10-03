#include "LevelSetup.hpp"
#include "TestFramework.hpp"
#include "bonuses/Falling.hpp"
#include "bonuses/Lying.hpp"

using namespace ugh;
using core::Fixed;

TEST(bonusCollectedOnce) {
    test::LevelSetup s;
    s.level.energy() = model::Energy(0x5a00);
    const data::BonusKind* energy = nullptr;
    for (const data::BonusKind* k : test::gameData().level(1, 0)->enemies[0].drops->items)   // the tree's
        if (k->effect == data::BonusKind::Effect::Energy) {
            energy = k;
            break;
        }
    CHECK(energy != nullptr);
    if (!energy) return;
    model::Copter& c = s.level.copter(0);
    bonuses::Falling::drop(s.level, *energy, c.x(), c.y(), Fixed(0), 0);
    model::BonusItem& b = s.level.bonuses()[11];
    CHECK(b.inUse());
    // lying at the copter
    b.changeState(bonuses::Lying::instance, s.level);
    b.moveToX(c.x() + Fixed::fromPixels(8));
    b.moveToY(c.y());
    s.level.bonuses().update(s.level);
    CHECK(!b.inUse());
    CHECK(s.level.energy().value() == model::Energy::FULL);   // filled up to the maximum, not over it
    CHECK(s.reported(core::EventKind::BonusCollected));
}
