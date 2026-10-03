#include "TestFramework.hpp"
#include "TestLevel.hpp"
#include "world/Raindrop.hpp"
#include "world/Water.hpp"

using namespace ugh;

TEST(the_water_moves_every_second_frame_and_rests_after_a_new_row) {
    world::Water water;
    water.fill(units::Fixed::fromRaw(100 * 32));
    water.move(units::Fixed::fromRaw(-20));   // first frame: no move
    CHECK_EQUAL(1, water.evenFrame());
    CHECK_EQUAL(100, water.row());
    water.move(units::Fixed::fromRaw(-20));   // second frame: up past a pixel row
    CHECK_EQUAL(99, water.row());
    CHECK(water.resting());
    water.move(units::Fixed::fromRaw(-20));   // the rest
    CHECK(!water.resting());
    CHECK_EQUAL(0, water.evenFrame());
}

TEST(the_water_surface_animation_runs_two_one_zero) {
    world::Water water;
    water.fill(units::Fixed());
    water.animateSurface();
    CHECK_EQUAL(2, water.surfaceFrame());
    CHECK_EQUAL(6, water.surfaceDelay());
    for (int i = 0; i < 7; i++) water.animateSurface();
    CHECK_EQUAL(1, water.surfaceFrame());
}

TEST(a_raindrop_blown_past_the_page_goes_on_in_the_next_row) {
    world::Raindrop drop{382, 10};
    world::Raindrop next = drop.blown(3, 1);
    CHECK_EQUAL(1, next.x);
    CHECK_EQUAL(14, next.y);
    next = world::Raindrop{1, 10}.blown(2, -1);
    CHECK_EQUAL(383, next.x);
    CHECK_EQUAL(11, next.y);
}

TEST(the_rain_starts_again_at_the_floor_row) {
    test::TestLevel t(100, data::Wind::Right);
    t.level().rain().setFloorRow(100);
    t.start();
    for (const world::Raindrop& drop : t.level().rain().drops()) CHECK(drop.y < 100);
}

TEST(the_last_passenger_ends_the_level) {
    test::TestLevel t;
    t.start();
    t.level().passengerFinished(t.events);
    CHECK(t.level().done());
    CHECK(t.level().fade().fadingOut());
    t.level().passengerFinished(t.events);
    CHECK_EQUAL(0, t.level().passengersLeft());
}
