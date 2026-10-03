#include <string>

#include "TestData.hpp"
#include "TestFramework.hpp"
#include "bonuses/BonusState.hpp"
#include "game/Game.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/standing/StandingPassenger.hpp"
#include "testing/TestPilot.hpp"

using namespace ugh;
using units::Fixed;
using units::Speed;

namespace {

/** The name of a passenger's state. */
class StateName : public passengers::PassengerVisitor {
public:
    std::string name, kind;
    void visit(const passengers::route::RoutePassenger& p) override {
        name = p.state().name();
        kind = p.kind().name;
    }
    void visit(const passengers::standing::StandingPassenger& p) override {
        name = p.state().name();
        kind = p.kind().name;
    }
};

std::string stateOf(const game::Game& g, int passenger) {
    StateName visitor;
    g.passengers()[passenger].accept(visitor);
    return visitor.name;
}

/** Level 1 of the one-player order, in its play (the caption skipped). */
void startLevelOne(game::Game& g) {
    g.newGame({});
    for (int frame = 0; frame < 80; frame++) g.step();
    g.menuKey(input::MenuKey::Other);
    while (g.phase() != game::GamePhase::Play) g.step();
}

/** The copter of player 0 put onto pad `pad` of the level. */
void landOn(game::Game& g, int pad) {
    const data::levels::PadDefinition& place = g.level().pad(pad).place();
    int x = (place.left + place.right) / 2 - 16, y = place.y - 20;
    testing::TestPilot(g).placeCopter(0, Fixed::fromPixels(x), Fixed::fromPixels(y), x, y, Speed(), Speed(), pad);
}

/** Steps until passenger `passenger` is in `state` (at most `frames`); false when it never gets there. */
bool stepUntil(game::Game& g, int passenger, const std::string& state, int frames, int pad = -1) {
    for (int frame = 0; frame < frames; frame++) {
        if (stateOf(g, passenger) == state) return true;
        if (pad >= 0) landOn(g, pad);   // the copter keeps standing there (the energy runs low otherwise)
        testing::TestPilot(g).setEnergy(20000);
        g.step();
    }
    return stateOf(g, passenger) == state;
}

}  // namespace

TEST(a_passenger_comes_out_rides_and_pays) {
    game::Game g(test::gameData());
    startLevelOne(g);
    landOn(g, 0);   // passenger 0 starts at pad 0 and wants to go to pad 2
    CHECK(stepUntil(g, 0, "Calling", 600, 0));
    CHECK(stepUntil(g, 0, "Riding", 600, 0));
    CHECK(g.level().copters()[0].cabin().cargo().has_value());
    CHECK_EQUAL(3, *g.level().copters()[0].cabin().cargo()->destination);   // the number of pad 2
    for (int frame = 0; frame < 50; frame++) {
        testing::TestPilot(g).setEnergy(20000);
        g.step();
    }
    CHECK_EQUAL(1100 - 50, g.level().copters()[0].cabin().fare());   // a unit less every frame of the ride
    landOn(g, 2);
    CHECK(stepUntil(g, 0, "WalkingToDoor", 5, 2));
    CHECK(g.session().score().points() > 0);
    CHECK(!g.level().copters()[0].cabin().cargo().has_value());
    CHECK(stepUntil(g, 0, "BehindDoor", 600, 2));   // through the door to its next stop
}

TEST(a_quick_delivery_drops_the_multiplier) {
    game::Game g(test::gameData());
    startLevelOne(g);
    landOn(g, 0);
    CHECK(stepUntil(g, 0, "Riding", 1200, 0));
    landOn(g, 2);
    g.step();
    CHECK(stepUntil(g, 0, "WalkingToDoor", 5, 2));
    int items = 0;
    for (int slot = 0; slot < bonuses::BonusSlots::SLOTS; slot++)
        if (g.bonuses()[slot]) items++;
    CHECK_EQUAL(1, items);
    CHECK(g.bonuses()[11].has_value());
    CHECK_EQUAL(std::string("multiplier"), g.bonuses()[11]->kind().name);
}

TEST(the_standing_passenger_is_carried_and_dropped) {
    game::Game g(test::gameData());
    startLevelOne(g);
    CHECK_EQUAL(std::string("Standing"), stateOf(g, 2));
    const passengers::Passenger& standing = g.passengers()[2];
    int x = standing.x().pixels() - 8, y = standing.y().pixels() - 8;
    testing::TestPilot(g).placeCopter(0, Fixed::fromPixels(x), Fixed::fromPixels(y), x, y, Speed(), Speed(), std::nullopt);
    g.step();
    CHECK_EQUAL(std::string("Hanging"), stateOf(g, 2));
    CHECK(!g.level().copters()[0].cabin().cargo()->destination.has_value());
    g.key(0, input::PlayerKey::Fire, true);
    g.step();
    CHECK_EQUAL(std::string("Falling"), stateOf(g, 2));
    CHECK(g.level().copters()[0].cabin().hasRoom());
}

TEST(a_passenger_knocked_into_the_water_drowns) {
    game::Game g(test::gameData());
    startLevelOne(g);
    landOn(g, 0);
    CHECK(stepUntil(g, 0, "Waiting", 600, 0));
    const passengers::Passenger& p = g.passengers()[0];
    int x = p.x().pixels() - 8, y = p.y().pixels() - 10;
    testing::TestPilot(g).placeCopter(0, Fixed::fromPixels(x), Fixed::fromPixels(y), x, y, Speed(), Speed(), std::nullopt);
    g.step();
    CHECK_EQUAL(std::string("Splash"), stateOf(g, 0));
    CHECK(g.level().pad(0).free());
    CHECK(stepUntil(g, 0, "Gone", 3000));
    StateName water;
    p.accept(water);
    CHECK_EQUAL(std::string("kind1-water"), water.kind);
}
