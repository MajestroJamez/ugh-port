#include "enemies/flyer/Flying.hpp"

#include "enemies/flyer/Falling.hpp"
#include "enemies/flyer/Flyer.hpp"
#include "enemies/flyer/Placed.hpp"
#include "physics/TouchBox.hpp"
#include "world/scenery/Screen.hpp"

namespace ugh::enemies::flyer {

namespace {

using units::Fixed;
using world::scenery::Screen;

// where it comes in: a pixel short of the edge past which it is gone
constexpr Fixed START_RIGHT = Screen::RIGHT - Fixed::fromPixels(1);
constexpr Fixed START_LEFT = Screen::FLYER_LEFT + Fixed::fromPixels(1);
constexpr Fixed HEIGHT = Fixed::fromPixels(26);        // it flies no lower than this above the water
constexpr Fixed CEILING = Fixed::fromPixels(-4);       // and no higher than this
constexpr int FLAP_DELAY = 4;

}  // namespace

const Flying Flying::instance{};

/** It takes its next target and comes in from the side away from the target's copter. */
void Flying::enter(Flyer& flyer, const EnemyContext& context) const {
    world::Level& level = context.level;
    const world::copter::Copter& copter = level.copters()[flyer.takeNextTarget(context.session.players())];
    if (copter.motion().x() < Screen::MIDDLE) {
        flyer.flyTowards(data::kinds::Facing::Left);
        flyer.moveToX(START_RIGHT);
    } else {
        flyer.flyTowards(data::kinds::Facing::Right);
        flyer.moveToX(START_LEFT);
    }
    Fixed y = copter.motion().y() + HEIGHT;
    if (y > level.water().level()) y = level.water().level();
    y -= HEIGHT;
    if (y < CEILING) y = CEILING;
    flyer.moveToY(y);
    context.report({events::EventKind::FlyerFlapStart, std::nullopt, flyer.index()});
}

void Flying::update(Flyer& flyer, const EnemyContext& context) const {
    Fixed x = flyer.x() + flyer.speedX();
    if (Screen::pastSide(x, Screen::FLYER_LEFT)) {
        context.report({events::EventKind::FlyerFlapStop, std::nullopt, flyer.index()});
        flyer.continueIn(Placed::instance, context);
        return;
    }
    flyer.moveToX(x);
    if (!flyer.animate(FLAP_DELAY)) return;
    flyer.show(flyer.kind().flight.towards(flyer.flight()));
    if (flyer.bounceFallingPassenger(context, Enemy::Rebound::Full)) {
        context.report({events::EventKind::FlyerFlapStop, std::nullopt, flyer.index()});
        flyer.changeState(Falling::instance, context);
        return;
    }
    const world::copter::Copter* copter =
        physics::TouchBox(flyer.kind().box, flyer.x(), flyer.y()).firstCopterIn(context.level.copters());
    if (copter && copter->player() == flyer.lastTarget()) context.level.crash(*copter, context.events);
}

}  // namespace ugh::enemies::flyer
