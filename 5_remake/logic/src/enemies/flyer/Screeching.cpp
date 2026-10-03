#include "enemies/flyer/Screeching.hpp"

#include "enemies/flyer/Flyer.hpp"
#include "enemies/flyer/Flying.hpp"

namespace ugh::enemies::flyer {

const Screeching Screeching::instance{};

void Screeching::enter(Flyer& flyer, const EnemyContext& context) const {
    flyer.startScreechTime(SCREECH_TIME);
    context.play.report({events::EventKind::FlyerScreech, std::nullopt, flyer.index()});
}

void Screeching::update(Flyer& flyer, const EnemyContext& context) const {
    if (flyer.screechTimeOver()) flyer.changeState(Flying::instance, context);
}

}  // namespace ugh::enemies::flyer
