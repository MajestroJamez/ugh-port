#include "enemies/flyer/Screeching.hpp"

#include "enemies/flyer/Flyer.hpp"
#include "enemies/flyer/Flying.hpp"

namespace ugh::enemies::flyer {

namespace {

constexpr int SCREECH_TIME = 70;   // frames

}  // namespace

const Screeching Screeching::instance{};

void Screeching::enter(Flyer& flyer, const EnemyContext& context) const {
    flyer.wait(SCREECH_TIME);
    context.report({events::EventKind::FlyerScreech, std::nullopt, flyer.index()});
}

void Screeching::update(Flyer& flyer, const EnemyContext& context) const {
    if (flyer.tickWait()) flyer.changeState(Flying::instance, context);
}

}  // namespace ugh::enemies::flyer
