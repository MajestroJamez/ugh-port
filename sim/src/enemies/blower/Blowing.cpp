#include "enemies/blower/Blowing.hpp"

#include "enemies/blower/BlowerWait.hpp"

namespace ugh::enemies {

namespace {

using core::Fixed;

constexpr core::Word FRAME_DELAY = 0x0f;
constexpr core::Word BLOW_FRAME = 3;     // the sound comes with this frame; the frames before it blow one way
constexpr core::Speed BLOW(0x29);        // the push per frame (1/64 Fixed per frame)
// where it blows, in pixels from its anchor: 9 .. 30 above, from 16 to 104 px further left (and the copter's width)
constexpr core::Word ZONE_BOTTOM = 9, ZONE_TOP = 30, ZONE_NEAR = 16, ZONE_FAR = 104 + 26;

}  // namespace

const Blowing Blowing::instance{};

/** 113b:295b */
void Blowing::enter(model::Enemy& enemy, model::Level&) const { enemy.restartAnimation(); }

/** 113b:2973 - blows: copters in front of it are pushed sideways, to and fro with the animation. */
void Blowing::update(model::Enemy& enemy, model::Level& level) const {
    const data::Box& box = enemy.kind().box;
    if (enemy.animate(FRAME_DELAY)) {
        if (enemy.animationFrame() == BLOW_FRAME) level.report({core::EventKind::BlowerBlow, core::Event::NONE, enemy.index()});
        enemy.show(*enemy.kind().charging.left);
    }
    Fixed bottom = enemy.y() + Fixed::fromPixels(box.y - ZONE_BOTTOM), top = enemy.y() + Fixed::fromPixels(box.y - ZONE_TOP);
    Fixed right = enemy.x() + Fixed::fromPixels(box.x - ZONE_NEAR), left = enemy.x() + Fixed::fromPixels(box.x - ZONE_FAR);
    for (int c = 0; c < level.copterCount(); c++) {
        model::Copter& copter = level.copter(c);
        if (bottom >= copter.y() && top <= copter.y() && right >= copter.x() && left <= copter.x()) {
            // the original compares the byte offset of the frame with 5
            copter.push(enemy.animationFrame() < BLOW_FRAME ? BLOW : -BLOW);
        }
    }
    if (bounceFallingPassenger(enemy, level, false)) enemy.changeState(BlowerWait::instance, level);
}

}  // namespace ugh::enemies
