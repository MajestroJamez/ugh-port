#include "enemies/blower/Stunned.hpp"

#include "enemies/blower/Blower.hpp"
#include "enemies/blower/Placed.hpp"

namespace ugh::enemies::blower {

const Stunned Stunned::instance{};

void Stunned::enter(Blower& blower, const EnemyContext& context) const {
    blower.scoreStun(blower.kind().score, context);
    blower.stun().start();
    blower.showSprite(blower.kind().stunnedSprite);
}

void Stunned::update(Blower& blower, const EnemyContext& context) const {
    if (blower.stun().over()) blower.continueIn(Placed::instance, context);
}

}  // namespace ugh::enemies::blower
