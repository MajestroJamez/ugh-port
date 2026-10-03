#include "enemies/blower/Blowing.hpp"

#include "enemies/blower/Blower.hpp"
#include "enemies/blower/Stunned.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::enemies::blower {

namespace {

using units::Fixed;

constexpr int FRAME_DELAY = 15;
constexpr int BLOW_FRAME = 3;   // the blow is heard with this frame; the frames before it blow one way, the rest back
constexpr units::Speed BLOW = units::Speed::fromRaw(41);   // the push per frame (1/64 Fixed per frame)
// where it blows, in pixels from its anchor: 9 .. 30 px above, 16 .. 104 px to the left (and the copter's width)
constexpr int ZONE_BOTTOM = 9, ZONE_TOP = 30, ZONE_NEAR = 16, ZONE_FAR = 104 + 26;

}  // namespace

const Blowing Blowing::instance{};

void Blowing::enter(Blower& blower, const EnemyContext&) const { blower.restartAnimation(); }

void Blowing::update(Blower& blower, const EnemyContext& context) const {
    const data::kinds::Box& box = blower.kind().box;
    if (blower.animate(FRAME_DELAY)) {
        if (blower.animator().frame() == BLOW_FRAME)
            context.report({events::EventKind::BlowerBlow, std::nullopt, blower.index()});
        blower.show(*blower.kind().blowing);
    }
    physics::TouchBox zone = physics::TouchBox::between(
        blower.x() + Fixed::fromPixels(box.x - ZONE_FAR), blower.x() + Fixed::fromPixels(box.x - ZONE_NEAR),
        blower.y() + Fixed::fromPixels(box.y - ZONE_TOP), blower.y() + Fixed::fromPixels(box.y - ZONE_BOTTOM));
    world::Level& level = context.level;
    for (world::Copter& copter : level.copters().all()) {
        if (zone.touches(copter))
            copter.setSpeed(copter.speedX() + (blower.animator().frame() < BLOW_FRAME ? BLOW : -BLOW), copter.speedY());
    }
    if (blower.bounceFallingPassenger(context, false)) blower.changeState(Stunned::instance, context);
}

}  // namespace ugh::enemies::blower
