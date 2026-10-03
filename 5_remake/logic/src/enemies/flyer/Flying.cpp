#include "enemies/flyer/Flying.hpp"

#include "enemies/flyer/Falling.hpp"
#include "enemies/flyer/Flyer.hpp"
#include "enemies/flyer/Placed.hpp"
#include "physics/TouchBox.hpp"
#include "world/Screen.hpp"

namespace ugh::enemies::flyer {

namespace {

using units::Fixed;

constexpr Fixed SCREEN_MIDDLE = Fixed::fromPixels(160);
constexpr Fixed START_RIGHT = Fixed::fromPixels(319), START_LEFT = Fixed::fromPixels(-31);   // just off the screen
constexpr Fixed HEIGHT = Fixed::fromPixels(26);        // it flies no lower than this above the water
constexpr Fixed CEILING = Fixed::fromPixels(-4);       // and no higher than this
constexpr units::Int16 FLAP_DELAY = 4;

}  // namespace

const Flying Flying::instance{};

/** It takes its next target and comes in from the side away from the target's copter. */
void Flying::enter(Flyer& flyer, const EnemyContext& context) const {
    world::Level& level = context.play.level;
    const world::Copter& copter = level.copter(flyer.takeNextTarget(context.play.session.players()));
    if (copter.x() < SCREEN_MIDDLE) {
        flyer.flyTowards(world::Facing::Left);
        flyer.moveToX(START_RIGHT);
    } else {
        flyer.flyTowards(world::Facing::Right);
        flyer.moveToX(START_LEFT);
    }
    Fixed y = copter.y() + HEIGHT;
    if (y > level.water().level()) y = level.water().level();
    y -= HEIGHT;
    if (y < CEILING) y = CEILING;
    flyer.moveToY(y);
    context.play.report({events::EventKind::FlyerFlapStart, std::nullopt, flyer.index()});
}

void Flying::update(Flyer& flyer, const EnemyContext& context) const {
    Fixed x = flyer.x() + flyer.speedX();
    if (world::Screen::pastSide(x, world::Screen::FLYER_LEFT)) {
        context.play.report({events::EventKind::FlyerFlapStop, std::nullopt, flyer.index()});
        flyer.continueIn(Placed::instance, context);
        return;
    }
    flyer.moveToX(x);
    if (!flyer.animate(FLAP_DELAY)) return;
    flyer.show(flyer.kind().flight.towards(flyer.flight() == world::Facing::Right));
    if (flyer.bounceFallingPassenger(context, true)) {
        context.play.report({events::EventKind::FlyerFlapStop, std::nullopt, flyer.index()});
        flyer.changeState(Falling::instance, context);
        return;
    }
    std::optional<int> copter = physics::TouchBox(flyer.kind().box, flyer.x(), flyer.y()).firstCopterIn(context.play.level);
    if (!copter || *copter != flyer.lastTarget() || context.play.level.fade().fadingOut()) return;
    context.play.level.fade().startFadeOut();
    context.play.report({events::EventKind::CopterCrashed, *copter});
}

}  // namespace ugh::enemies::flyer
