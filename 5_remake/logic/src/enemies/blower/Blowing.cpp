#include "enemies/blower/Blowing.hpp"

#include "enemies/blower/Blower.hpp"
#include "enemies/blower/Stunned.hpp"
#include "physics/TouchBox.hpp"
#include "world/copter/CopterShape.hpp"

namespace ugh::enemies::blower {

namespace {

using units::Fixed;

constexpr int FRAME_DELAY = 15;
constexpr int BLOW_FRAME = 3;   // the blow is heard with this frame; the frames before it blow one way, the rest back
constexpr units::Speed BLOW = units::Speed::fromRaw(41);   // the push per frame (1/64 Fixed per frame)
// where it blows, in pixels from its anchor: ZONE_BOTTOM .. ZONE_TOP above, ZONE_NEAR .. ZONE_FAR to the left (the far
// edge 104 px and the copter up to the right edge of its body, as the zone is where the copter's top left corner is)
constexpr int ZONE_BOTTOM = 9, ZONE_TOP = 30, ZONE_NEAR = 16, ZONE_FAR = 104 + world::copter::CopterShape::BODY_RIGHT;

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
    for (world::copter::Copter& copter : level.copters().all()) {
        if (zone.touches(copter))
            copter.motion().setSpeedX(copter.motion().speedX() + (blower.animator().frame() < BLOW_FRAME ? BLOW : -BLOW));
    }
    if (blower.bounceFallingPassengerUnseen(context)) blower.changeState(Stunned::instance, context);
}

}  // namespace ugh::enemies::blower
