#include "TestFramework.hpp"
#include "TestLevel.hpp"
#include "physics/CollisionProbe.hpp"
#include "physics/CopterPhysics.hpp"

using namespace ugh;
using units::Fixed;
using units::Speed;

namespace {

void fly(test::TestLevel& t, int frames) {
    world::PlayContext c = t.context();
    physics::CopterPhysics physics(c);
    for (int i = 0; i < frames; i++) physics.fly(0);
}

}  // namespace

TEST(a_copter_falls_and_lands_softly_on_a_pad) {
    test::TestLevel t;
    for (int x = 100; x <= 200; x++) t.solid(x, 150);   // the surface of the pad
    t.start();
    world::Copter& copter = t.level().copter(0);
    copter.placeByTestPilot(Fixed::fromPixels(140), Fixed::fromPixels(124), 140, 124, Speed(), Speed(), std::nullopt);
    fly(t, 40);
    CHECK(copter.landedOn(0));
    CHECK(copter.speedY() == Speed());
    CHECK_EQUAL(130, copter.pixelY().value());   // the skids (20 px below the top) on the surface
    CHECK(!t.level().fade().fadingOut());
}

TEST(a_copter_that_hits_the_ground_too_hard_crashes) {
    test::TestLevel t;
    for (int x = 0; x < 320; x++) t.solid(x, 150);
    t.start();
    world::Copter& copter = t.level().copter(0);
    copter.placeByTestPilot(Fixed::fromPixels(10), Fixed::fromPixels(100), 10, 100, Speed(), Speed::fromRaw(6000),
                            std::nullopt);
    fly(t, 20);
    CHECK(t.level().fade().fadingOut());
    auto events = t.events.take();
    CHECK_EQUAL(1, static_cast<int>(events.size()));
    CHECK(events[0].kind == events::EventKind::CopterCrashed);
}

TEST(a_copter_under_water_floats_up_to_the_surface) {
    test::TestLevel t(120);
    t.start();
    world::Copter& copter = t.level().copter(0);
    copter.placeByTestPilot(Fixed::fromPixels(10), Fixed::fromPixels(110), 10, 110, Speed(), Speed(), std::nullopt);
    fly(t, 200);
    CHECK_EQUAL(0, copter.depthIn(t.level().water().row()).value());
    CHECK(copter.stillVertically());
}

TEST(the_wind_blows_a_copter_sideways) {
    test::TestLevel t(190, data::Wind::Left);
    t.start();
    world::Copter& copter = t.level().copter(0);
    Fixed before = copter.x();
    fly(t, 10);
    CHECK(copter.x() < before);
}

TEST(the_probe_going_left_looks_only_one_pixel_ahead) {
    test::TestLevel t;
    for (int y = 0; y < 192; y++) t.solid(100, y);   // a wall one pixel thin
    t.start();
    world::Copter copter;
    copter.placeByTestPilot(Fixed::fromPixels(100), Fixed::fromPixels(50), 100, 50, Speed(), Speed(), std::nullopt);
    physics::CollisionProbe probe(t.level());
    // the probe origin is 5 px right of the copter: the wall is 5 px to its left
    CHECK(!probe.stopOnTheWay(copter, physics::CollisionProbe::Axis::Horizontal, copter.x(), copter.x() - Fixed::fromPixels(8)));
    // going right it sweeps every pixel: the wall at x 100 stops the copter that starts at 70 (origin 75, outline to 95)
    copter.placeByTestPilot(Fixed::fromPixels(70), Fixed::fromPixels(50), 70, 50, Speed(), Speed(), std::nullopt);
    auto stop = probe.stopOnTheWay(copter, physics::CollisionProbe::Axis::Horizontal, copter.x(), copter.x() + Fixed::fromPixels(20));
    CHECK(stop.has_value());
    CHECK_EQUAL(74, stop->pixels().value());
}
