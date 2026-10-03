#include "LevelSetup.hpp"
#include "TestFramework.hpp"
#include "physics/TouchBox.hpp"

using namespace ugh;
using core::Fixed;

TEST(copterTouchesASpriteOnlyWhenItOverlapsIt) {
    test::LevelSetup s;
    model::Copter& c = s.level.copter(0);
    const data::Box box{10, 20, 5, 10};   // anchor 10, 20; 10 x 20 px around it
    const Fixed x = Fixed::fromPixels(100), y = Fixed::fromPixels(100);
    physics::TouchBox touch(box, x, y);

    c.moveToX(x);
    c.moveToY(y);
    CHECK(touch.touches(c));
    CHECK(touch.firstCopterIn(s.level) == 0);

    // the box spans x + 5 .. x + 15 across: the copter's body (5 .. 26 from its corner) reaches it from 21 px left
    c.moveToX(x - Fixed::fromPixels(21));
    CHECK(touch.touches(c));
    c.moveToX(x - Fixed::fromPixels(22));
    CHECK(!touch.touches(c));
    CHECK(touch.firstCopterIn(s.level) == model::Level::NONE);
}
