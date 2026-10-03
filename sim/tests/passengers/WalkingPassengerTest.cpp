#include "LevelSetup.hpp"
#include "TestFramework.hpp"

using namespace ugh;
using core::Fixed;
using core::Word;

TEST(passengerRidesAndPays) {
    test::LevelSetup s;
    CHECK(s.passengerState(0) == "NextStop");
    CHECK(s.runPassengersUntil(0, "Waiting"));
    model::Passenger& p = s.level.passenger(0);
    int pickup = p.pickupPad(), target = p.targetPad();
    CHECK(!s.level.pad(pickup).free());
    // a copter lands next to the passenger: it calls, then walks to it and boards
    s.land(0, pickup, s.level.pad(pickup).waitX() - 0x20);
    CHECK(s.runPassengersUntil(0, "Calling", 600));
    CHECK(p.snapshot().bubble != data::NO_SPRITE);
    CHECK(s.runPassengersUntil(0, "Riding", 2000));
    const model::Copter::Snapshot& copter = s.level.copter(0).snapshot();
    CHECK(copter.carrying == p.kind().look);
    CHECK(copter.targetPad == s.level.pad(target).number());
    CHECK(s.level.pad(pickup).free());
    CHECK(s.reported(core::EventKind::PassengerBoarded));
    // at the target it pays the fare (one less after this frame, times the multiplier 1)
    Word fare = s.level.copter(0).fare();
    s.land(0, target, s.level.copter(0).pixelX());
    s.level.updatePassengers();
    CHECK(s.passengerState(0) == "WalkingAway");
    CHECK(s.level.session().snapshot().score == static_cast<uint32_t>(fare.value() - 1));
    CHECK(s.level.copter(0).hasRoom());
    CHECK(s.reported(core::EventKind::PassengerPaid));
    // a quick delivery drops a bonus item for the multiplier
    const model::BonusItem& bonus = s.level.bonuses()[11];
    CHECK(bonus.inUse() && &bonus.kind() == &test::gameData().quickDeliveryBonus());
}

TEST(passengerKnockedIntoWater) {
    test::LevelSetup s;
    CHECK(s.runPassengersUntil(0, "Waiting"));
    model::Passenger& p = s.level.passenger(0);
    // a copter flies right through it
    model::Copter& c = s.level.copter(0);
    c.takeOff();
    c.moveToX(p.x() - Fixed::fromPixels(8));
    c.moveToY(p.y() - Fixed::fromPixels(8));
    s.level.updatePassengers();
    CHECK(s.passengerState(0) == "Splash");
    CHECK(p.kind().type == data::PassengerKind::Type::Swimming);
    CHECK(s.reported(core::EventKind::PassengerInWater));
}
