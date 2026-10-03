#include "enemies/blower/Blowing.hpp"

#include "enemies/blower/Blower.hpp"
#include "enemies/blower/Stunned.hpp"

namespace ugh::enemies::blower {

namespace {

using units::Fixed;

constexpr units::Int16 FRAME_DELAY = 15;
constexpr int BLOW_FRAME = 3;   // the blow is heard with this frame; the frames before it blow one way, the rest back
constexpr units::Speed BLOW = units::Speed::fromRaw(41);   // the push per frame (1/64 Fixed per frame)
// where it blows, in pixels from its anchor: 9 .. 30 px above, 16 .. 104 px to the left (and the copter's width)
constexpr units::Int16 ZONE_BOTTOM = 9, ZONE_TOP = 30, ZONE_NEAR = 16, ZONE_FAR = 104 + 26;

}  // namespace

const Blowing Blowing::instance{};

void Blowing::enter(Blower& blower, const EnemyContext&) const { blower.restartAnimation(); }

void Blowing::update(Blower& blower, const EnemyContext& context) const {
    const data::Box& box = blower.kind().box;
    if (blower.animate(FRAME_DELAY)) {
        if (blower.animator().frame() == BLOW_FRAME) context.play.report({events::EventKind::BlowerBlow, std::nullopt, blower.index()});
        blower.show(*blower.kind().blowing);
    }
    Fixed bottom = blower.y() + Fixed::fromPixels(box.y - ZONE_BOTTOM), top = blower.y() + Fixed::fromPixels(box.y - ZONE_TOP);
    Fixed right = blower.x() + Fixed::fromPixels(box.x - ZONE_NEAR), left = blower.x() + Fixed::fromPixels(box.x - ZONE_FAR);
    world::Level& level = context.play.level;
    for (int c = 0; c < level.copterCount(); c++) {
        world::Copter& copter = level.copter(c);
        if (bottom >= copter.y() && top <= copter.y() && right >= copter.x() && left <= copter.x())
            copter.push(blower.animator().frame() < BLOW_FRAME ? BLOW : -BLOW);
    }
    if (blower.bounceFallingPassenger(context, false)) blower.changeState(Stunned::instance, context);
}

}  // namespace ugh::enemies::blower
