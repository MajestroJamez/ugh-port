#include "enemies/flyer/Hidden.hpp"

#include "enemies/flyer/Flyer.hpp"
#include "enemies/flyer/Screeching.hpp"

namespace ugh::enemies::flyer {

const Hidden Hidden::instance{};

void Hidden::enter(Flyer& flyer, const EnemyContext&) const {
    flyer.restartAnimation();
    flyer.hide();
    flyer.wait(flyer.startDelay());
}

void Hidden::update(Flyer& flyer, const EnemyContext& context) const {
    if (flyer.tickWait()) flyer.changeState(Screeching::instance, context);
}

}  // namespace ugh::enemies::flyer
