#include "LevelSetup.hpp"
#include "TestFramework.hpp"
#include "physics/CopterPhysics.hpp"

using namespace ugh;

TEST(copterFallsOntoPad) {
    test::LevelSetup s;
    model::Copter& c = s.level.copter(0);
    s.hover(0, 8);
    physics::CopterPhysics physics(s.level);
    for (int f = 0; f < 200 && !c.landed(); f++) physics.fly(0);
    CHECK(c.landedOn(1));
    CHECK(c.speedX() == core::Speed(0) && c.speedY() == core::Speed(0));
    CHECK(!s.level.fade().fadingOut());   // a soft touch-down
}

TEST(hardImpactCrashes) {
    test::LevelSetup s;
    model::Copter& c = s.level.copter(0);
    s.hover(0, 1);
    c.setSpeed(core::Speed(0), core::Speed(0x1800));
    physics::CopterPhysics(s.level).fly(0);
    CHECK(s.level.fade().fadingOut());
    CHECK(s.reported(core::EventKind::CopterCrashed));
}
