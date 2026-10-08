#include <string>

#include "TestFramework.hpp"
#include "TestLevel.hpp"
#include "physics/CopterDanger.hpp"
#include "physics/CopterPhysics.hpp"
#include "testing/TestPilot.hpp"

using namespace ugh;
using physics::CopterDanger;
using physics::CopterPhysics;
using units::Fixed;
using units::Speed;

namespace {

constexpr data::Difficulty DIFFICULTIES[] = {data::Difficulty::Easy, data::Difficulty::Medium, data::Difficulty::Hard};
constexpr Speed GRAVITY = Speed::fromRaw(27);   // a frame in the air pulls the copter down so much faster

/** A wall of rock across the screen's height from column `left` to `right`. */
void wall(test::TestLevel& t, int left, int right) {
    for (int x = left; x <= right; x++)
        for (int y = 0; y < 192; y++) t.solid(x, y);
}

/** A floor (or ceiling) of rock across the screen from row `top` to `bottom`. */
void slab(test::TestLevel& t, int top, int bottom) {
    for (int y = top; y <= bottom; y++)
        for (int x = 0; x < 320; x++) t.solid(x, y);
}

/**
 * The copter at pixel x, y going vx, vy: at the end of its pixel the way it goes (right or down; left or up at its
 * start), so that any speed moves it into the next pixel in a frame - into what is beside it.
 */
void place(test::TestLevel& t, int x, int y, Speed vx, Speed vy) {
    const int endX = vx > Speed() ? 31 : 0, endY = vy > Speed() ? 31 : 0;
    testing::TestPilot::placeCopter(t.level().copters()[0], Fixed::fromRaw(x * 32 + endX), Fixed::fromRaw(y * 32 + endY), x,
                                    y, vx, vy, nullptr);
}

CopterDanger danger(test::TestLevel& t) {
    return CopterDanger::of(t.level(), t.session().crashLimit(), t.level().copters()[0]);
}

/** Frames of flight without a key; true when the copter crashed. */
bool crashes(test::TestLevel& t, int frames) {
    world::PlayContext c = t.context();
    physics::CopterPhysics physics(c);
    for (int i = 0; i < frames && !t.level().fade().fadingOut(); i++) physics.fly(t.level().copters()[0]);
    return t.level().fade().fadingOut();
}

std::string what(data::Difficulty d, const char* way, int speed) {
    return std::string(way) + " difficulty " + std::to_string(static_cast<int>(d)) + " speed " + std::to_string(speed);
}

}  // namespace

TEST(the_crash_limits_of_the_difficulties_are_the_originals) {
    // (the impact of a bounce: the speed rounded to even - up going right or down, down going left or up)
    CHECK_EQUAL(3100, test::TestLevel(190, data::levels::Wind::None, data::Difficulty::Easy).session().crashLimit());
    CHECK_EQUAL(2300, test::TestLevel(190, data::levels::Wind::None, data::Difficulty::Medium).session().crashLimit());
    CHECK_EQUAL(1380, test::TestLevel(190, data::levels::Wind::None, data::Difficulty::Hard).session().crashLimit());
    CHECK_EQUAL(2300, CopterPhysics::impactOf(Speed::fromRaw(2299)));
    CHECK_EQUAL(2298, CopterPhysics::impactOf(Speed::fromRaw(-2299)));
    CHECK_EQUAL(6144, CopterPhysics::impactOf(CopterPhysics::TOP_SPEED));
}

// the danger says a crash exactly when the logic crashes the copter into rock at that speed, on every difficulty and
// either way across (no key, no wind: the speed of the bounce is the speed it had)
TEST(across_the_danger_is_a_crash_exactly_above_the_limit) {
    for (data::Difficulty d : DIFFICULTIES) {
        const int limit = test::TestLevel(190, data::levels::Wind::None, d).session().crashLimit();
        for (int speed = limit - 4; speed <= limit + 3; speed++) {
            for (int way : {1, -1}) {
                test::TestLevel t(190, data::levels::Wind::None, d);
                wall(t, 200, 204);
                wall(t, 100, 104);
                t.start();
                // beside the wall to the right (its outline's right side at 199) or to the left (its left side at 105)
                place(t, way > 0 ? 174 : 100, 50, Speed::fromRaw(way * speed), Speed());
                const CopterDanger before = danger(t);
                CHECK(before.across.rock);
                CHECK_EQUAL(limit, before.crashLimit);
                const bool crashed = crashes(t, 1);
                if (crashed != before.crashes(before.across))
                    test::Registry::instance().fail(what(d, way > 0 ? "right" : "left", speed));
                // going right a speed of the limit less one crashes already, going left the limit
                CHECK_EQUAL(crashed, way * speed >= limit - 1 || -way * speed >= limit);
            }
        }
    }
}

// down onto a floor (a frame in the air adds gravity: the copter starts that much slower) and up into a ceiling of rock
TEST(up_and_down_the_danger_is_a_crash_exactly_above_the_limit) {
    for (data::Difficulty d : DIFFICULTIES) {
        const int limit = test::TestLevel(190, data::levels::Wind::None, d).session().crashLimit();
        for (int speed = limit - 4; speed <= limit + 3; speed++) {
            for (int way : {1, -1}) {
                test::TestLevel t(190, data::levels::Wind::None, d);
                slab(t, 150, 155);   // a floor: its outline's bottom at 149 over it
                slab(t, 10, 20);     // a ceiling: its top at 21 under it
                t.start();
                const int y = way > 0 ? 130 : 21;
                const Speed hit = Speed::fromRaw(way * speed);
                place(t, 10, y, Speed(), hit);
                const CopterDanger at = danger(t);
                CHECK(at.upDown.rock);
                place(t, 10, y, Speed(), hit - GRAVITY);
                const bool crashed = crashes(t, 1);
                if (crashed != at.crashes(at.upDown)) test::Registry::instance().fail(what(d, way > 0 ? "down" : "up", speed));
            }
        }
    }
}

TEST(the_edges_of_the_screen_and_the_open_sky_are_no_danger) {
    for (data::Difficulty d : DIFFICULTIES) {
        test::TestLevel t(190, data::levels::Wind::None, d);
        t.start();
        // at the top speed across and up into nothing but the edges: they stop it, no crash
        place(t, 250, 60, CopterPhysics::TOP_SPEED, -CopterPhysics::TOP_SPEED);
        const CopterDanger right = danger(t);
        CHECK(right.across.impact >= right.crashLimit && right.upDown.impact >= right.crashLimit);
        CHECK(!right.across.rock && !right.upDown.rock);
        CHECK(!right.crashes(right.across) && !right.crashes(right.upDown));
        CHECK(!crashes(t, 60));
        place(t, 30, 60, -CopterPhysics::TOP_SPEED, Speed());
        CHECK(!danger(t).across.rock);
        CHECK(!crashes(t, 60));
    }
}

TEST(falling_into_deep_water_is_no_danger_into_shallow_water_it_is) {
    for (data::Difficulty d : DIFFICULTIES) {
        // the surface at 120 (it floats with its top at 102), the bottom far below
        test::TestLevel deep(120, data::levels::Wind::None, d);
        slab(deep, 186, 191);
        deep.start();
        place(deep, 10, 40, Speed(), CopterPhysics::TOP_SPEED);
        const CopterDanger falling = danger(deep);
        CHECK(falling.upDown.impact >= falling.crashLimit);
        CHECK(!falling.upDown.rock);
        CHECK(!crashes(deep, 200));
        // a bottom 6 px under the surface: the water cannot brake it in time
        test::TestLevel shallow(120, data::levels::Wind::None, d);
        slab(shallow, 126, 191);
        shallow.start();
        place(shallow, 10, 40, Speed(), CopterPhysics::TOP_SPEED);
        CHECK(danger(shallow).upDown.rock);
        CHECK(crashes(shallow, 200));
        // under water going up it stops at the surface: no danger from the sky above it
        test::TestLevel under(120, data::levels::Wind::None, d);
        slab(under, 10, 20);
        under.start();
        place(under, 10, 110, Speed(), -CopterPhysics::TOP_SPEED);
        CHECK(!danger(under).upDown.rock);
    }
}
