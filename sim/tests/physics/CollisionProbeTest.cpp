#include <optional>

#include "LevelSetup.hpp"
#include "TestFramework.hpp"
#include "physics/CollisionProbe.hpp"

using namespace ugh;
using core::Fixed;
using Axis = physics::CollisionProbe::Axis;

TEST(probeStopsOnTheSurfaceOfAPad) {
    test::LevelSetup s;
    s.hover(0, 8);   // the skids 8 px above pad 1
    const model::Copter& c = s.level.copter(0);
    std::optional<Fixed> stop =
        physics::CollisionProbe(s.level).stopOnTheWay(c, Axis::Vertical, c.y(), c.y() + Fixed::fromPixels(20));
    CHECK(stop.has_value());
    CHECK(stop && stop->pixels() + 0x14 == s.level.pad(1).y());   // the skids (20 px down) on the surface
}

TEST(probeLetsTheCopterMoveThroughTheAir) {
    test::LevelSetup s;
    s.hover(0, 8);
    const model::Copter& c = s.level.copter(0);
    physics::CollisionProbe probe(s.level);
    CHECK(!probe.stopOnTheWay(c, Axis::Vertical, c.y(), c.y() + Fixed::fromPixels(4)));
    CHECK(!probe.stopOnTheWay(c, Axis::Vertical, c.y(), c.y() - Fixed::fromPixels(1)));
}
