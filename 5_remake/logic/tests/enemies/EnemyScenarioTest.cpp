#include <string>

#include "TestData.hpp"
#include "TestFramework.hpp"
#include "TestLevel.hpp"
#include "data/levels/BlowerPlacement.hpp"
#include "data/levels/FlyerPlacement.hpp"
#include "data/levels/WalkerPlacement.hpp"
#include "enemies/blower/Blower.hpp"
#include "enemies/flyer/Flyer.hpp"
#include "enemies/tree/Tree.hpp"
#include "enemies/walker/Walker.hpp"
#include "game/Game.hpp"
#include "testing/TestPilot.hpp"

using namespace ugh;
using units::Fixed;
using units::Speed;

namespace {

/** A test level with nobody in it but the enemy under test. */
struct Arena {
    test::TestLevel level;
    passengers::Passengers passengers;
    bonuses::BonusSlots bonuses;

    Arena() { level.start(); }

    void run(enemies::Enemy& enemy, int frames) {
        world::PlayContext play = level.context();
        enemies::EnemyContext context{play, passengers, bonuses};
        for (int i = 0; i < frames; i++) enemy.update(context);
    }
};

}  // namespace

TEST(the_flyer_hunts_the_copter_and_ends_the_attempt) {
    Arena arena;
    enemies::flyer::Flyer flyer(0, test::gameData().flyerKind(), data::levels::FlyerPlacement(3, Fixed::fromRaw(50)));
    arena.run(flyer, 1);
    CHECK_EQUAL(std::string("Hidden"), std::string(flyer.state().name()));
    arena.run(flyer, 3);
    CHECK_EQUAL(std::string("Screeching"), std::string(flyer.state().name()));
    arena.run(flyer, 70);
    CHECK_EQUAL(std::string("Flying"), std::string(flyer.state().name()));
    CHECK_EQUAL(0, flyer.lastTarget());
    CHECK(flyer.flight() == world::figure::Facing::Left);   // the copter is on the left half: from the right
    CHECK(flyer.y() == arena.level.level().copters()[0].motion().y());
    arena.run(flyer, 200);
    CHECK(arena.level.level().fade().fadingOut());
}

TEST(the_walker_watches_charges_and_throws_the_copter) {
    Arena arena;
    world::copter::Copter& copter = arena.level.level().copters()[0];
    testing::TestPilot::placeCopter(copter, Fixed::fromPixels(170), Fixed::fromPixels(130), 170, 130, Speed(), Speed(), &arena.level.level().pad(0));
    enemies::walker::Walker walker(0, test::gameData().walkerKind(),
                                   data::levels::WalkerPlacement(0, Fixed::fromPixels(110), Fixed::fromPixels(128), Fixed::fromRaw(-34)),
                                   arena.level.level().pad(0));
    arena.run(walker, 2);
    CHECK_EQUAL(std::string("Watching"), std::string(walker.state().name()));
    arena.run(walker, 140);
    CHECK_EQUAL(std::string("Charging"), std::string(walker.state().name()));
    CHECK(walker.facing() == world::figure::Facing::Right);
    for (int frame = 0; frame < 100 && copter.landed(); frame++) arena.run(walker, 1);
    CHECK(!copter.landed());
    CHECK(copter.motion().speedY() > Speed());
    CHECK_EQUAL(std::string("Recovering"), std::string(walker.state().name()));
}

TEST(the_blower_pushes_a_copter_in_front_of_it) {
    Arena arena;
    world::copter::Copter& copter = arena.level.level().copters()[0];
    enemies::blower::Blower blower(0, test::gameData().blowerKind(),
                                   data::levels::BlowerPlacement(Fixed::fromPixels(200), Fixed::fromPixels(100)));
    // the zone: 9 .. 30 px above the blower's anchor (y 100 + 22), 16 .. 130 px to the left of it (x 200 + 16)
    testing::TestPilot::placeCopter(copter, Fixed::fromPixels(150), Fixed::fromPixels(100), 150, 100, Speed(), Speed(), nullptr);
    arena.run(blower, 2);
    CHECK(copter.motion().speedX() != Speed());
}

TEST(a_passenger_dropped_onto_the_tree_shakes_out_a_bonus_item) {
    game::Game g(test::gameData());
    g.newGame({});
    for (int frame = 0; frame < 80; frame++) g.step();
    g.menuKey(input::MenuKey::Other);
    while (g.phase() != game::GamePhase::Play) g.step();
    for (int frame = 0; frame < 100; frame++) g.step();   // past the fade-in: the copters fly
    const passengers::Passenger& standing = g.passengers()[2];
    int x = standing.x().pixels() - 8, y = standing.y().pixels() - 8;
    testing::TestPilot(g).placeCopter(0, Fixed::fromPixels(x), Fixed::fromPixels(y), x, y, Speed(), Speed(), std::nullopt);
    g.step();   // it hangs below the copter
    testing::TestPilot(g).placeCopter(0, Fixed::fromPixels(60), Fixed::fromPixels(40), 60, 40, Speed(), Speed(), std::nullopt);
    g.key(0, input::PlayerKey::Fire, true);   // let it go above the tree
    bool dropped = false;
    for (int frame = 0; frame < 60 && !dropped; frame++) {
        g.step();
        for (int slot = 0; slot < bonuses::BonusSlots::SLOTS; slot++) dropped = dropped || g.bonuses()[slot].has_value();
    }
    CHECK(dropped);
    CHECK(g.bonuses()[11].has_value());
    CHECK_EQUAL(std::string("energy1"), g.bonuses()[11]->kind().name);
}
